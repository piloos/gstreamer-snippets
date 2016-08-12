CC=gcc
CFLAGS=-I.

current_dir = $(shell pwd)

$(info starting make)
$(info current working directory: ${current_dir})

appsink_localsource: appsink_localsource.c
	$(CC) -Os -Wall -o appsink_localsource.o \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include/gstreamer-1.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include/glib-2.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/lib/glib-2.0/include \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include/gstreamer-1.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/lib/gstreamer-1.0/include \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include/c++/5.3.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include/c++/5.3.0/x86_64-buildroot-linux-gnu \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include/c++/5.3.0/backward \
  -I${current_dir}/../../host/usr/lib/gcc/x86_64-buildroot-linux-gnu/5.3.0/include \
  -I${current_dir}/../../host/usr/lib/gcc/x86_64-buildroot-linux-gnu/5.3.0/include-fixed \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include \
  -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgstapp-1.0 -lgstvideo-1.0 \
  appsink_localsource.c

appsink_v4l2: appsink_v4l2.c
	$(CC) -Os -Wall -o appsink_v4l2.o \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include/gstreamer-1.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include/glib-2.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/lib/glib-2.0/include \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include/gstreamer-1.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/lib/gstreamer-1.0/include \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include/c++/5.3.0 \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include/c++/5.3.0/x86_64-buildroot-linux-gnu \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include/c++/5.3.0/backward \
  -I${current_dir}/../../host/usr/lib/gcc/x86_64-buildroot-linux-gnu/5.3.0/include \
  -I${current_dir}/../../host/usr/lib/gcc/x86_64-buildroot-linux-gnu/5.3.0/include-fixed \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/include \
  -I${current_dir}/../../host/usr/x86_64-buildroot-linux-gnu/sysroot/usr/include \
  -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgstapp-1.0 -lgstvideo-1.0 \
  appsink_v4l2.c

clean:
	rm -f *.o
