#include <stdio.h>
#include <ulib.h>

int magic = -0x10384;
int var = -0x10385;
char padding[4096] = { 1 };
int var2 = 5;

int
main(void) {
    int pid, code;
    cprintf("I am the parent. Forking the child...\n");
    if ((pid = fork()) == 0) {
        var = -0x58301;
        cprintf("I am the child. Forking the child...\n");
        yield();
        yield();
        yield();
        int pid2;
        if ((pid2 = fork()) == 0) {
            cprintf("I am the child's child.\n");
            var = +0x58301;
            var2 = 111;
            exit(0xcafe);
        }
        cprintf("I am the child, fork a child pid %d\n", pid2);
        yield();
        yield();
        yield();
        yield();
        assert(var == -0x58301);
        assert(var2 == 5);
        assert(waitpid(pid2, &code) == 0 && code == 0xcafe);
        assert(waitpid(pid2, &code) != 0 && wait() != 0);
        exit(magic);
    }
    var = -0x10385;
    cprintf("I am parent, fork a child pid %d\n", pid);
    yield();
    yield();
    yield();
    yield();
    assert(pid > 0);
    cprintf("I am the parent, waiting now..\n");

    assert(var == -0x10385);
    assert(var2 == 5);
    assert(waitpid(pid, &code) == 0 && code == magic);
    assert(waitpid(pid, &code) != 0 && wait() != 0);
    cprintf("waitpid %d ok.\n", pid);
    var2 = 0;
    cprintf("exit pass.\n");
    return 0;
}

