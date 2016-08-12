#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void simple_pipeline(void);
void appsink_pipeline(const char* format);
void appsink_pipeline_with_callbacks(const char* format);

int main(int argc, char *argv[]) {
    printf("Kicking off...\n");

    gst_init(&argc, &argv);

    //simple_pipeline();
    //appsink_pipeline(argv[1]);
    appsink_pipeline_with_callbacks(argv[1]);

    gst_deinit();

    printf("...the end!\n");

    return 0;
}

void simple_pipeline() {
    GstElement *pipeline;

    pipeline = gst_parse_launch("v4l2src ! autovideosink", NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    printf("Press ENTER to stop the pipeline...\n");
    getchar();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void appsink_pipeline(const char* format) {
    char pipeline_string[500];
    GstElement *pipeline, *appsink;
    GError *error = NULL;
    GstSample *sample;
    guint max_buffers;
    GstCaps *caps;
    const GstStructure *info;
    gchar *caps_string, *info_string;
    GstBuffer *buffer;
    gsize bufsize;

    sprintf(pipeline_string, "v4l2src ! appsink name=mysink caps=\"video/x-raw,format=%s,framerate=30/1\"", format);

    pipeline = gst_parse_launch(pipeline_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");

    max_buffers = gst_app_sink_get_max_buffers((GstAppSink*) appsink);

    printf("Maximum amount of buffers in appsink (0=unlimited): %d\n", max_buffers);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    while(1) {
        sample = gst_app_sink_pull_sample((GstAppSink*) appsink);
        printf("\nNew frame!\n");
        caps = gst_sample_get_caps(sample);
        caps_string = gst_caps_to_string(caps);
        printf("  Caps: %s\n", caps_string);
        g_free(caps_string);
        info = gst_sample_get_info(sample);
        if(info != NULL) {
            info_string = gst_structure_to_string(info);
            printf("  Info: %s\n", info_string);
            g_free(info_string);
        }
        else
            printf("  No extra info.\n");
        buffer = gst_sample_get_buffer(sample);
        printf("  PTS: %ld\n", GST_TIME_AS_MSECONDS(buffer->pts));
        printf("  DTS: %ld\n", buffer->dts);
        printf("  Duration [ms]: %ld\n", GST_TIME_AS_MSECONDS(buffer->duration));
        printf("  Frame number: %ld\n", buffer->offset);
        bufsize = gst_buffer_get_size(buffer);
        printf("  Size: %ld\n", bufsize);

        gst_sample_unref(sample);
    }

    printf("Press ENTER to stop the pipeline...\n");
    getchar();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
}

GstFlowReturn new_sample_callback(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample;
    guint max_buffers;
    GstCaps *caps;
    const GstStructure *info;
    gchar *caps_string, *info_string;
    GstBuffer *buffer;
    gsize bufsize;

    printf("\nNew frame!\n");
    sample = gst_app_sink_pull_sample(appsink);
    caps = gst_sample_get_caps(sample);
    caps_string = gst_caps_to_string(caps);
    printf("  Caps: %s\n", caps_string);
    g_free(caps_string);
    info = gst_sample_get_info(sample);
    if(info != NULL) {
        info_string = gst_structure_to_string(info);
        printf("  Info: %s\n", info_string);
        g_free(info_string);
    }
    else
        printf("  No extra info.\n");
    buffer = gst_sample_get_buffer(sample);
    printf("  PTS: %ld\n", GST_TIME_AS_MSECONDS(buffer->pts));
    printf("  DTS: %ld\n", buffer->dts);
    printf("  Duration [ms]: %ld\n", GST_TIME_AS_MSECONDS(buffer->duration));
    printf("  Frame number: %ld\n", buffer->offset);
    bufsize = gst_buffer_get_size(buffer);
    printf("  Size: %ld\n", bufsize);

    gst_sample_unref(sample);

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

void appsink_pipeline_with_callbacks(const char* format) {
    char pipeline_string[500];
    GstElement *pipeline, *appsink;
    GError *error = NULL;
    GstAppSinkCallbacks my_callbacks;

    sprintf(pipeline_string, "v4l2src ! appsink name=mysink caps=\"video/x-raw,format=%s,framerate=30/1\"", format);

    pipeline = gst_parse_launch(pipeline_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");

    my_callbacks.new_sample = new_sample_callback;
    my_callbacks.new_preroll = new_preroll_callback;
    my_callbacks.eos = eos_callback;

    gst_app_sink_set_callbacks((GstAppSink*) appsink, &my_callbacks, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    printf("Press ENTER to stop the pipeline...\n");
    getchar();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
}
