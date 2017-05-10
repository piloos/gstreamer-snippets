#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/gstvideoutils.h>
#include <gst/audio/audio-info.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helpers/hexString.h"

void appsink_pipeline(const char* filelocation);

int main(int argc, char *argv[]) {
    printf("Kicking off...\n");

    gst_init(&argc, &argv);

    appsink_pipeline(argv[1]);

    gst_deinit();

    printf("...the end!\n");

    return 0;
}

GstFlowReturn throw_away_callback(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample;
    sample = gst_app_sink_pull_sample(appsink);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

GstFlowReturn new_video_sample_callback(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample;
    GstCaps *caps;
    gchar *caps_string, *content;
    GstBuffer *buffer;
    gsize bufsize;
    GstMapInfo map;
    gboolean isWhite;

    sample = gst_app_sink_pull_sample(appsink);

    buffer = gst_sample_get_buffer(sample);

    if(gst_buffer_map(buffer, &map, GST_MAP_READ) != TRUE) {
        printf("Could not map video buffer\n");
        return GST_FLOW_OK;
    }

    content = bytesToHexString(map.data, 10);
    //printf("  Content:\n%s\n", content);
    isWhite = (strcmp(content, "0000000000") == 0) ? FALSE : TRUE;
    if(isWhite == TRUE) {
        printf("AV7 video ");
        printf("%ld ", GST_TIME_AS_MSECONDS(buffer->pts));
        printf("white\n");
    }
    free(content);

    gst_buffer_unmap(buffer, &map);

    gst_sample_unref(sample);

    //important to return some kind of OK message, otherwise the pipe will block
    return GST_FLOW_OK;
}

gint32 checkSound(const gchar* content, gint32 samples_per_ms)
{
    guint32 length = strlen(content);
    guint32 i;
    //printf("  length %d\n", length);
    for(i = 0; i < length; i += samples_per_ms*2) {
        if( strncmp(content+i, "00", 2) !=0 )
            return i/samples_per_ms/2;
    }
    return -1;
}

GstFlowReturn new_audio_sample_callback(GstAppSink *appsink, gpointer user_data)
{
    GstSample *sample;
    GstCaps *caps;
    gchar *caps_string, *content;
    GstBuffer *buffer;
    gsize bufsize;
    GstMapInfo map;
    gboolean hasSound;
    gint32 soundStart;
    gint32 absoluteSoundStart;
    guint32 frames_per_ms;
    GstAudioInfo audioInfo;

    sample = gst_app_sink_pull_sample(appsink);

    caps = gst_sample_get_caps(sample);
    //caps_string = gst_caps_to_string(caps);
    //printf("  Caps: %s\n", caps_string);
    //g_free(caps_string);
    gst_audio_info_from_caps(&audioInfo, caps);
    frames_per_ms = audioInfo.bpf * audioInfo.rate / 1000;
    //printf("%d\n", frames_per_ms);

    buffer = gst_sample_get_buffer(sample);

    if(gst_buffer_map(buffer, &map, GST_MAP_READ) != TRUE) {
        printf("Could not map audio buffer\n");
        return GST_FLOW_OK;
    }

    //printf("  Size: %ld\n", map.size);
    content = bytesToHexString(map.data, map.size * 2);
    //printf("  Content:\n%s\n", content);
    soundStart = checkSound(content, frames_per_ms);
    if(soundStart < 0) {
        hasSound = FALSE;
    }
    else {
        absoluteSoundStart = GST_TIME_AS_MSECONDS(buffer->pts) + soundStart;
        hasSound = TRUE;
    }
    if(hasSound == TRUE) {
        printf("AV7 audio ");
        printf("%ld ", GST_TIME_AS_MSECONDS(buffer->pts));
        printf("%d ", absoluteSoundStart);
        printf("noise\n");
    }
    free(content);

    gst_buffer_unmap(buffer, &map);

    gst_sample_unref(sample);

    //important to return some kind of OK message, otherwise the pipe will block
    return GST_FLOW_OK;
}

void eos_callback(GstAppSink *appsink, gpointer user_data)
{
    printf("\nEOS reached!!\n");
}

void appsink_pipeline(const char* filelocation) {
    char pipeline_string[500];
    GstElement *pipeline, *appsink;
    GError *error = NULL;
    GstAppSinkCallbacks my_callbacks;

    sprintf(pipeline_string, "filesrc location=%s ! qtdemux name=m m.video_0 ! queue ! avdec_h264 ! videoconvert ! appsink name=video_appsink caps=\"video/x-raw,format=RGB\" sync=false m.audio_0 ! queue ! aacparse ! faad ! appsink name=audio_appsink caps=\"audio/x-raw\" sync=false", filelocation);

    printf("\n\nGST pipeline: %s\n\n", pipeline_string);

    pipeline = gst_parse_launch(pipeline_string, &error);

    if(error != NULL) {
        printf("ERROR: could not construct pipeline: %s\n", error->message);
        g_clear_error(&error);
        exit(-1);
    }

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "video_appsink");

    my_callbacks.new_sample = new_video_sample_callback;
    //my_callbacks.new_sample = throw_away_callback;
    my_callbacks.new_preroll = NULL;
    my_callbacks.eos = eos_callback;

    gst_app_sink_set_callbacks((GstAppSink*) appsink, &my_callbacks, NULL, NULL);

    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "audio_appsink");

    my_callbacks.new_sample = new_audio_sample_callback;
    my_callbacks.new_preroll = NULL;
    my_callbacks.eos = eos_callback;

    gst_app_sink_set_callbacks((GstAppSink*) appsink, &my_callbacks, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    printf("Press ENTER to stop the pipeline...\n");
    getchar();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);
}
