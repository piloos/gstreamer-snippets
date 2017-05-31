#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


GTimer *timer;

void appsink_pipeline(const char* filelocation);

int main(int argc, char *argv[]) {
    guint major, minor, micro, nano;

    printf("Kicking off...\n\n");

    gst_init(&argc, &argv);

    gst_version(&major, &minor, &micro, &nano);
    printf("Gstreamer version %d.%d.%d.%d\n", major, minor, micro, nano);
    if (major != 1 || minor < 6) {
        printf("This application needs at least gstreamer version 1.6.3 to work reliably.\n");
        return -1;
    }

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
    GstBuffer *buffer;
    GstFlowReturn ret;
    gdouble pushtime;
    guint size;
    GstMapInfo map;
    guint i;
    static gboolean silence = TRUE;
    static guint silence_count = 0;

    sample = gst_app_sink_pull_sample(appsink);
    caps = gst_sample_get_caps(sample);
    caps_string = gst_caps_to_string(caps);
    printf("Caps: %s\n", caps_string);
    g_free(caps_string);

    buffer = gst_sample_get_buffer(sample);

    size = gst_buffer_get_size(buffer);
    printf("Buffer passing in appsink, size %d, pts %d\n", size, GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer)));

    if(gst_buffer_map(buffer, &map, GST_MAP_READ) != TRUE) {
        printf("Could not map audio buffer\n");
        return GST_FLOW_OK;
    }

    for (i=0; i<map.size; ++i) {
        if (silence) {
            if (map.data[i] != 0) {
                printf("Found something else than 0 in buffer with pts %d, %d bytes into the buffer\n", GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer)), i);
                silence = FALSE;
                break;
            }
        }
        else {
            if (map.data[i] == 0) {
                ++silence_count;
                if (silence_count > 200) {
                    printf("Found again a 0 in the buffer with pts %d, %d bytes into the buffer\n", GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer)), i-silence_count);
                    silence = TRUE;
                    silence_count = 0;
                }
            }
            else {
                silence_count = 0;
            }
        }
    }

    //for (i=0; i<map.size; ++i) {
    //    printf("%02X",(unsigned char)map.data[i]);
    //}
    printf("\n\n");

    gst_buffer_unmap(buffer, &map);


    /* we don't need the appsink sample anymore */
    gst_sample_unref (sample);

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

static GstPadProbeReturn inspect_audio_buffer(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);

    printf("Buffer %s, size %d, pts %d\n", (unsigned char*) user_data, gst_buffer_get_size(buffer), GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer)));
    return GST_PAD_PROBE_OK;
}

static gboolean print_pipeline_callback(gpointer data)
{
    GstElement *pipeline = (GstElement*) data;
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline1");
    return FALSE; //returning FALSE makes sure that we are only called once
}

void appsink_pipeline(const char* filelocation) {
    char pipeline_string[500];
    GstElement *pipeline, *appsink, *el;
    GError *error = NULL;
    GstAppSinkCallbacks my_callbacks;
    GstPad *pad;
    GMainLoop *loop;

    loop = g_main_loop_new ( NULL , FALSE );

    timer = g_timer_new();

    /*
     * +---------+       +-------+       +---------+       +------------+
     * |         |       |       | audio |         |       |            |    sync=false: appsink will push
     * | filesrc +-------> demux +-------> decoder +-------> appsink    |    buffers into appsrc as soon
     * |         |       |       |       |         |       | sync=false |    as it receives the buffers
     * +---------+       +-------+       +---------+       +------------+
     *
     */

    sprintf(pipeline_string, "filesrc location=%s ! qtdemux name=demux demux.audio_0 ! faad name=mydec ! appsink name=mysink sync=false", filelocation);

    printf("\nGST pipeline 1: %s\n", pipeline_string);

    pipeline = gst_parse_launch(pipeline_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline 1: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    el = gst_bin_get_by_name(GST_BIN(pipeline), "mydec");
    pad = gst_element_get_static_pad(el, "sink");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) inspect_audio_buffer, "incoming on decoder", NULL);
    gst_object_unref(pad);
    pad = gst_element_get_static_pad(el, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) inspect_audio_buffer, "outgoing from decoder", NULL);
    gst_object_unref(pad);
    gst_object_unref(el);

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");

    my_callbacks.new_sample = new_sample_callback;
    my_callbacks.new_preroll = new_preroll_callback;
    my_callbacks.eos = eos_callback;

    gst_app_sink_set_callbacks((GstAppSink*) appsink, &my_callbacks, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_timeout_add (1000 , print_pipeline_callback , (gpointer) pipeline);

    g_main_loop_run (loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);

    g_main_loop_unref(loop);
}
