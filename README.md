# QSV解码+SDL2.0播放

目录

- 目录
- 编译信息
- 说明

## 编译信息

作者：xzf

创建时间：2021/08/13

更新时间：2021/12/30

## 说明
最新说明：由于SDL最新的版本增加了SDL_UpdateNVTexture函数。原来的播放函数已经不对，此库不再更新，请参考https://github.com/chemoontheshy/learn-FFmpeg

本文主要说明，QSV解码+SDL2.0播放

- qsv，主要解mp4，其实也可以解码rtsp.（主要看这个就好）
  - 需要下载ffmpeg库
  - sdl库
- qsvdec官方代码，无法直接执行。
- qsvSDL,采用直接解码RTP码流，所以解码方式不一样，需要用到jrtplib库，实际上最好不要采用这个，可以直接采用socket。
- 
