#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/gstvideoutils.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void appsink_pipeline(const char* filelocation);

int main(int argc, char *argv[]) {
    printf("Kicking off...\n");

    gst_init(&argc, &argv);

    appsink_pipeline(argv[1]);

    gst_deinit();

    printf("...the end!\n");

    return 0;
}

GstFlowReturn new_sample_callback(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample;
    GstCaps *caps;
    gchar *caps_string;
    GstBuffer *buffer, *app_buffer;
    GstFlowReturn ret;
    GstElement *appsrc;

    printf("\nNew frame!\n");
    sample = gst_app_sink_pull_sample(appsink);
    caps = gst_sample_get_caps(sample);
    caps_string = gst_caps_to_string(caps);
    //printf("  Caps: %s\n", caps_string);
    g_free(caps_string);

    buffer = gst_sample_get_buffer(sample);

    /* make a copy (no deep copy if not necessary) */
    app_buffer = gst_buffer_copy (buffer);
    printf("Buffer size %d\n", gst_buffer_get_size(buffer));

    /* we don't need the appsink sample anymore */
    gst_sample_unref (sample);

    /* push new buffer to appsrc */
    appsrc = (GstElement*) user_data;
    gst_app_src_set_caps ( GST_APP_SRC(appsrc), caps);
    ret = gst_app_src_push_buffer ( GST_APP_SRC(appsrc) , app_buffer);

    //important to return some kind of OK message, otherwise the pipe will block
    return GST_FLOW_OK;
}

GstFlowReturn new_preroll_callback(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample;

    printf("\nNew preroll sample!\n");
    sample = gst_app_sink_pull_preroll(appsink);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

void eos_callback(GstAppSink *appsink, gpointer user_data)
{
    printf("\nEOS reached!!\n");
}

static GstPadProbeReturn inspect_video_buffer(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);

    printf("Buffer passing %s, size %d, pts %d\n", (unsigned char*) user_data, gst_buffer_get_size(buffer), GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer)));
    return GST_PAD_PROBE_OK;
}

static gboolean print_pipeline_callback(gpointer data)
{
    GstElement *pipeline = (GstElement*) data;
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline1");
    return FALSE; //returning FALSE makes sure that we are only called once
}

static gboolean print_pipeline2_callback(gpointer data)
{
    GstElement *pipeline = (GstElement*) data;
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline2");
    return FALSE; //returning FALSE makes sure that we are only called once
}

static gboolean start_pipeline_callback(gpointer data)
{
    GstElement *pipeline = (GstElement*) data;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    return FALSE;
}

void appsink_pipeline(const char* filelocation) {
    char pipeline_string[500], pipeline2_string[500];
    GstElement *pipeline, *appsink, *pipeline2, *appsrc, *el;
    GError *error = NULL;
    GstAppSinkCallbacks my_callbacks;
    GstPad *pad;
    GMainLoop *loop;

    loop = g_main_loop_new ( NULL , FALSE );

    sprintf(pipeline_string, "filesrc location=%s ! qtdemux name=demux demux.video_0 ! queue name=myqueue ! appsink name=mysink sync=true", filelocation);
    sprintf(pipeline2_string, "appsrc name=mysource ! queue ! avdec_h264 name=mydec ! videoconvert ! ximagesink");

    printf("\n\nGST pipeline 1: %s\n\n", pipeline_string);
    printf("\n\nGST pipeline 2: %s\n\n", pipeline2_string);

    //configuring pipeline 2
    pipeline2 = gst_parse_launch(pipeline2_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline 2: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    appsrc = gst_bin_get_by_name(GST_BIN(pipeline2), "mysource");

    el = gst_bin_get_by_name(GST_BIN(pipeline2), "mydec");
    pad = gst_element_get_static_pad(el, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) inspect_video_buffer, "incoming on decoder", NULL);
    gst_object_unref(pad);
    gst_object_unref(el);

    //configuring pipeline 1
    pipeline = gst_parse_launch(pipeline_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline 1: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    el = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    pad = gst_element_get_static_pad(el, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) inspect_video_buffer, "incoming on appsink", NULL);
    gst_object_unref(pad);
    //pad = gst_element_get_static_pad(el, "src");
    //gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) inspect_video_buffer, "outgoing", NULL);
    //gst_object_unref(pad);
    gst_object_unref(el);

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");

    my_callbacks.new_sample = new_sample_callback;
    my_callbacks.new_preroll = new_preroll_callback;
    my_callbacks.eos = eos_callback;

    gst_app_sink_set_callbacks((GstAppSink*) appsink, &my_callbacks, (gpointer) appsrc, NULL);

    //start both pipelines
    gst_element_set_state(pipeline2, GST_STATE_PLAYING);
    gst_element_set_state(pipeline, GST_STATE_PAUSED);

    g_timeout_add (1000 , start_pipeline_callback, (gpointer) pipeline);

    g_timeout_add (2000 , print_pipeline_callback , (gpointer) pipeline);
    g_timeout_add (2000 , print_pipeline2_callback , (gpointer) pipeline2);

    g_main_loop_run (loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(pipeline2, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(appsrc);
    gst_object_unref(pipeline);
    gst_object_unref(pipeline2);

    g_main_loop_unref(loop);
}
