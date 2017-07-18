#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/gstvideoutils.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void pipeline(const char* filelocation, int rate_numerator, int rate_denominator);

int main(int argc, char *argv[]) {
    printf("Kicking off...\n");

    gst_init(&argc, &argv);

    pipeline(argv[1], atoi(argv[2]), atoi(argv[3]));

    gst_deinit();

    printf("...the end!\n");

    return 0;
}

static GstPadProbeReturn inspect_video_buffer(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER (info);

    printf("Buffer %s, size %d, pts %d\n", (unsigned char*) user_data, gst_buffer_get_size(buffer), GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer)));
    return GST_PAD_PROBE_OK;
}

gboolean print_pipeline_callback(gpointer data)
{
    GstElement *pipeline = (GstElement*) data;
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "basic_pipeline");
    return FALSE; //returning FALSE makes sure that we are only called once
}

void pipeline(const char* filelocation, int rate_numerator, int rate_denominator) {
    char pipeline_string[500];
    GstElement *pipeline, *el;
    GError *error = NULL;
    GstPad *pad;
    GMainLoop *loop;
    GstClock *clock;

    loop = g_main_loop_new ( NULL , FALSE );

    sprintf(pipeline_string, "filesrc location=%s ! qtdemux name=demux demux.video_0 ! queue name=myqueue ! avdec_h264 name=mydec ! videoconvert ! ximagesink name=mysink", filelocation);

    printf("\n\nGST pipeline 1: %s\n\n", pipeline_string);

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
    gst_object_unref(el);

    //init pipeline to paused
    gst_element_set_state(pipeline, GST_STATE_PAUSED);

    //change the clock
    printf("\nModifying clock speed by factor %d/%d\n\n", rate_numerator, rate_denominator);
    clock = gst_element_provide_clock(pipeline);
    assert(clock != NULL);
    GstClockTime external = gst_clock_get_time(clock);
    GstClockTime internal = gst_clock_get_internal_time(clock);
    gst_clock_set_calibration(clock, internal, external, rate_numerator, rate_denominator); //modify clock speed by factor (rate_numerator / rate_denominator)
    gst_object_unref(clock);

    //start pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_timeout_add (1000 , print_pipeline_callback , (gpointer) pipeline);

    g_main_loop_run (loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    g_main_loop_unref(loop);
}
