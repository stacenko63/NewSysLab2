#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <vector>
using namespace std;
void sigaction_handle(int signum, siginfo_t *info, void* ctx);
int pid1, pid2, result = 0, count = 1, module = 10;
bool isParentWish = true;
volatile sig_atomic_t last_sig;
volatile sig_atomic_t sig_value;
vector<bool> numbers(21, false);
struct sigaction act;
void sigaction_handle(int signum, siginfo_t *info, void* ctx) {
        sig_value = info->si_value.sival_int;
        last_sig = signum;
}
void make_a_number() {        //загадать число
    result = rand() % 21 + 1;
    cout << "Загаданное число: " << result << "\n";
}
int get_value() {
    int index = rand() % 21 + 1;
    while (numbers[index]) {
        index = rand() % 21 + 1;
    }
    numbers[index] = true;
    return index;
}
void unravel_a_number() {    //отгадать число
    clock_t t = clock();
    while (true) {
        int tmp = get_value();
        cout << "Попытка " << count++ << ": " << tmp << "\n";
        if (pid2 == 0) {
            sigqueue(pid1, SIGRTMIN, sigval{tmp});
        }
        else {
            sigqueue(pid2, SIGRTMIN, sigval{tmp});
        }
        pause();
        if (last_sig == SIGUSR1) {
            if (pid2 == 0) cout << "Дочерний процесс угадал число!\n";
            else cout << "Родительский процесс угадал число!\n";
            cout << "Количество попыток: " << count - 1 << "\n";
            cout << "Затраченное время: " << double(clock() - t) / CLOCKS_PER_SEC << "\n";
            count = 1;
            for (int i = 0; i < numbers.size(); i++) {
                numbers[i] = false;
            }
            if (pid2 == 0) {
                sigqueue(pid1, SIGRTMIN, sigval{tmp});
            }
            else {
                sigqueue(pid2, SIGRTMIN, sigval{tmp});
            }
            break;
        }
    }
}
void work(bool flag) {
    if ((isParentWish && flag) || (!isParentWish && !flag)) {
        if (flag) {
            cout << "Загадывает родительский процесс, угадывает дочерний процесс\nРодительский процесс начал работу!\n";
        }
        else {
            cout << "Загадывает дочерний процесс, угадывает родительский\nДочерний процесс начал работу!\n";
        }
        make_a_number();
        if (flag) {
            sigqueue(pid2, SIGUSR2, sigval{});
        }
        else {
            sigqueue(pid1, SIGUSR2, sigval{});
        }
        while (true) {
            pause();
            if (sig_value == result) {
                if (flag) {
                    sigqueue(pid2, SIGUSR1, sigval{});
                }
                else {
                    sigqueue(pid1, SIGUSR1, sigval{});
                }
                break;
            }
            else {
                if (flag) {
                    sigqueue(pid2, SIGUSR2, sigval{});
                }
                else {
                    sigqueue(pid1, SIGUSR2, sigval{});
                }
            }
        }
        pause();
        if (flag) isParentWish = false;
        else isParentWish = true;
    }
    else
    {
        if (flag) cout << "Родительский процесс начал работу!\nЖдем сигнал от дочернего процесса!\n";
        else cout << "Дочерний процесс начал работу!\nЖдем сигнал от родительского процесса!\n";
        while (last_sig != SIGUSR2) {}
        cout << "Сигнал получен!\n";
        unravel_a_number();
        if (flag) isParentWish = true;
        else isParentWish = false;
    }
}
int main() {
    srand(11 + time(NULL));
    pid1 = getpid();
    act.sa_sigaction = sigaction_handle;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &act, 0);
    sigaction(SIGUSR1, &act, 0);
    sigaction(SIGRTMIN, &act, 0);
    cout << "Родительский процесс: " << pid1 << "\n";
    pid2 = fork();
    for (int i = 0; i <= 2; i++) {
        if (pid2 != 0) { //родительский процесс
            work(true);
        }
        else { //дочерний процесс
            srand(time(NULL));
            cout << "Дочерний процесс: " << getpid() << "\n";
            work(false);
        }
    }
}