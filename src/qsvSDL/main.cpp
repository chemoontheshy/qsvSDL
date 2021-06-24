#include "qdecode.h"


int main(int argc, char* argv[])
{
   /* QDecode qDecode;
    const char* input = "../../3rd/video/test.mp4";
    qDecode.setUrl(input);
    qDecode.play();*/
    //JRTPLIB jrtp;
    QDecode qDecode;
    qDecode.setPortbase(4002);
    qDecode.play();
    return 0;
}