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

    //printf("\nNew frame!\n");
    sample = gst_app_sink_pull_sample(appsink);
    caps = gst_sample_get_caps(sample);
    caps_string = gst_caps_to_string(caps);
    //printf("  Caps: %s\n", caps_string);
    g_free(caps_string);

    buffer = gst_sample_get_buffer(sample);

    /* make a copy (no deep copy if not necessary) */
    app_buffer = gst_buffer_copy (buffer);
    //printf("Buffer size %d\n", gst_buffer_get_size(buffer));
    
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

    //printf("\nBuffer passing!\n");
    //printf("Buffer size %d\n", gst_buffer_get_size(buffer));
    return GST_PAD_PROBE_OK;
}

void appsink_pipeline(const char* filelocation) {
    char pipeline_string[500], pipeline2_string[500];
    GstElement *pipeline, *appsink, *pipeline2, *appsrc, *tocheck;
    GError *error = NULL;
    GstAppSinkCallbacks my_callbacks;
    GstPad *pad;

    sprintf(pipeline_string, "filesrc location=%s ! qtdemux name=demux demux.video_0 ! queue ! appsink name=mysink sync=true", filelocation);
    sprintf(pipeline2_string, "appsrc name=mysource ! queue ! avdec_h264 ! videoconvert ! ximagesink name=tocheck");

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

    tocheck = gst_bin_get_by_name(GST_BIN(pipeline2), "tocheck");
    pad = gst_element_get_static_pad(tocheck, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) inspect_video_buffer, NULL, NULL);
    gst_object_unref(pad);

    //configuring pipeline 1
    pipeline = gst_parse_launch(pipeline_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline 1: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");

    my_callbacks.new_sample = new_sample_callback;
    my_callbacks.new_preroll = new_preroll_callback;
    my_callbacks.eos = eos_callback;

    gst_app_sink_set_callbacks((GstAppSink*) appsink, &my_callbacks, (gpointer) appsrc, NULL);

    //start both pipelines
    gst_element_set_state(pipeline2, GST_STATE_PLAYING);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    printf("Press ENTER to stop the pipeline...\n");
    getchar();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(pipeline2, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(appsrc);
    gst_object_unref(pipeline);
    gst_object_unref(pipeline2);
}
