#include "qdecode.h"
#include "jrtplib.h"

int main(int argc, char* argv[])
{
   /* QDecode qDecode;
    const char* input = "../../3rd/video/test.mp4";
    qDecode.setUrl(input);
    qDecode.play();*/
    JRTPLIB jrtp;
    uint16_t portbase = 4002;
    jrtp.setPort(portbase);
    jrtp.getVersion();
    return 0;
}