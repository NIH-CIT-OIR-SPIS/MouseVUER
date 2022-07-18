## Command for server side for one will fix later will have in local server

ffmpeg -listen 1 -timeout 10000 -f flv -loglevel debug -an -i rtmp://127.0.0.1:5000/ -c:v copy -pix_fmt yuv420p10le -y Testing_DIR/test_lsb.mp4

ffmpeg -listen 1 -timeout 10000 -f flv -loglevel debug -an -i rtmp://127.0.0.1:5001/ -c:v copy -pix_fmt rgb24 -y Testing_DIR/test_msb.mp4


## Code on listening side
 "ffmpeg " + banner + " -y " + thread_counter + " -f rawvideo -pix_fmt " + pix_fmt + " -c:v rawvideo -s " + std::to_string(width) + "x" + std::to_string(height) + " -t " + std::to_string(time_run) + " -r " + std::to_string(fps) + " -an -i -  -c:v " + encoder + " -preset veryfast -pix_fmt " + pix_fmt_out + " -crf " + std::to_string(crf) + " -x264-params bframes=" + std::to_string(num_bframes) + " -movflags +faststart -f flv rtmp://127.0.0.1:5000/ ";