using size_t = unsigned int;
using ssize_t = int;

using ContinuationFunction = void (*)(int);

union Word {
    size_t as_uint;
    ssize_t as_int;
};

extern ContinuationFunction function_exit;
extern int arg0;
extern int arg1;
extern int arg2;
extern int arg3;

extern "C" {

void add_const(int x) {
    function_exit(x + arg0);
}

void mult_const(int x) {
    function_exit(x * arg0);
}

void div_const(int x) {
    function_exit(x / arg0);
}

}
