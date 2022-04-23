
/***************************************************************
 * GMP(The GNU Multi Presicion Arithmetic Library) Test by C++.
 **************************************************************/
#include <iostream>  // for cout
#include <gmp.h>
#include <gmpxx.h>

using namespace std;

class TestGmp
{
    // Declaration
    mpz_class op1, op2, op3, op5, op6, op7;  // GMP Integer
    mpz_class rop;                           // GMP Integer
    unsigned int op4;                        // Unsigned integer

public:
    TestGmp();        // Constructor
    void execTest();  // Execute test
};

/*
 * Constructor
 */
TestGmp::TestGmp()
{
    // Initialization
    op1 = 123456789;   // GMP Integer
    op2 = 987654;      // GMP Integer
    op3 = 2;           // GMP Integer
    op4 = 100;         // Unsigned integer
    op5 = 123456789;   // GMP Integer
    op6 = 987654;      // GMP Integer
    op7 = -999999999;  // GMP Integer
}

/*
 * Execute test
 */
void TestGmp::execTest()
{
    // Addition
    rop = op1 + op2;
    cout << op1 << " + " << op2 << " = " << rop << endl;

    // Subtraction
    rop = op1 - op2;
    cout << op1 << " - " << op2 << " = " << rop << endl;

    // Multiplication
    rop = op1 * op2;
    cout << op1 << " * " << op2 << " = " << rop << endl;

    // Division(Remaider)
    rop = op5 / op6;
    cout << op5 << " / " << op6 << " = " << rop << endl;

    // Modulo
    rop = op5 % op6;
    cout << op5 << " % " << op6 << " = " << rop << endl;

    // Left shift
    rop = op3 << op4;
    cout << op3 << " << " << op4 << " = " << rop << endl;

    // Absolute value
    rop = abs(op7);
    cout << "abs(" << op7 << ") = " << rop << endl;

    // Square root
    rop = sqrt(op1);
    cout << "sqrt(" << op1 << ") = " << rop << endl;
}

int main()
{
    try
    {
        // Instantiation
        TestGmp objGmp;

        // Execute test
        objGmp.execTest();
    }
    catch (...) {
        cout << "ERROR!" << endl;
        return -1;
    }

    cerr << "is everything alright?" << endl;
    return 0;
}
