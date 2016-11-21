#include <gst/gst.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sync_pipeline(const char* filelocation);

int main(int argc, char *argv[]) {
    printf("Kicking off...\n");

    gst_init(&argc, &argv);

    sync_pipeline(argv[1]);

    gst_deinit();

    printf("...the end!\n");

    return 0;
}

void buffer_passing(GstElement *identity, GstBuffer *buffer, gpointer user_data)
{
    //printf("Buffer passing! PTS: %ld\n", buffer->pts);
    buffer->pts += 500000000LL;
}

void sync_pipeline(const char* filelocation) {
    char pipeline_string[500];
    GstElement *pipeline, *identity;
    GError *error = NULL;

    sprintf(pipeline_string, "filesrc location=%s ! qtdemux name=m m.video_0 ! queue ! avdec_h264 ! videoconvert ! ximagesink sync=FALSE"
                             " m.audio_0 ! queue ! aacparse ! faad ! identity name=idvideo ! alsasink device=plughw:0,1 sync=FALSE", filelocation);

    printf("\n\nGST pipeline: %s\n\n", pipeline_string);

    pipeline = gst_parse_launch(pipeline_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    identity = gst_bin_get_by_name(GST_BIN(pipeline), "idvideo");

    //g_signal_connect(identity, "handoff", G_CALLBACK(buffer_passing), NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    printf("Press ENTER to stop the pipeline...\n");
    getchar();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(identity);
}
