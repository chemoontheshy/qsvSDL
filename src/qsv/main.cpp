#include "qdecode.h"

int main(int argc, char* argv[])
{
    QDecode qDecode;
    const char* input = "udp://127.0.0.1:4002";
    qDecode.setUrl(input);
    qDecode.play();
    return 0;
}