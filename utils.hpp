#include <gmpxx.h>

enum PackType {TYPE_UNKNOWN, TYPE_COMPUTE, TYPE_COMBINE, TYPE_COMBINE2};

struct PQT {
    mpz_class P, Q, T;
};

class ReqPack {
    int id_;
    int n1_;
    int n2_;
    PackType type_;
    mpz_class a_, b_, c_, d_;
    mpf_class fa_, fb_;
public:
    ReqPack();
    ReqPack(int id, int n1, int n2);
    ReqPack(int id, mpz_class& a, mpz_class& b);
    ReqPack(int id, mpf_class& fa);
    ReqPack(int id, mpf_class& fa, mpf_class& fb);
    ReqPack(int id, mpz_class& a, mpz_class& b, mpz_class& c, mpz_class& d);
    ReqPack(int id, mpz_class&& a, mpz_class&& b, mpz_class&& c, mpz_class&& d);
    int GetID();
    int GetN1();
    int GetN2();
    PackType GetType();
    mpz_class Geta();
    mpz_class Getb();
    mpz_class Getc();
    mpz_class Getd();
    mpf_class Getfa();
    mpf_class Getfb();
    void Invalidate();
    bool IsValid();
};

class RespPack {
    int id_;
    int n1_;
    int n2_;
    PQT result_;
    PackType type_;
    mpz_class a_;
    mpf_class fa_;
public:
    RespPack();
    RespPack(int id, int n1, int n2, PQT& result);
    RespPack(int id, int n1, int n2, PQT&& result);
    RespPack(ReqPack& rp, PQT& result);
    RespPack(ReqPack& rp, PQT&& result);
    RespPack(int id, mpz_class& a);
    RespPack(ReqPack& rp, mpz_class& a);
    RespPack(ReqPack& rp, mpf_class& fa);
    int GetID();
    int GetN1();
    int GetN2();
    PQT GetResult();
    PackType GetType();
    mpz_class Geta();
    mpf_class Getfa();
    void Invalidate();
    bool IsValid();
};



void ClockStart();
void ClockEnd(int ms);
void SetCpuAffinity(int cpu_no, std::thread& thread_obj);
void SetCpuAffinity(int cpu_no);
uint32_t GetBatchNum(int num_of_cores);
