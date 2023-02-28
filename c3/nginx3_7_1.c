#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char* const* argv)
{
    write(STDIN_FILENO, "aaaabbb", 6);
    return 0;
}
