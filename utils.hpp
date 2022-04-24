#include <memory>
#include <gmpxx.h>

enum PackType {TYPE_UNKNOWN, TYPE_MINIMAL, TYPE_COMPUTE, TYPE_COMBINE, TYPE_COMBINE2};

struct NativePQT {
    mpz_class P, Q, T;
};

struct PQT {
    std::shared_ptr<mpz_class> P, Q, T;
};

class ReqPack {
    int id_;
    int n1_;
    int n2_;
    PackType type_;
    std::shared_ptr<mpz_class> a_, b_, c_, d_;
    std::shared_ptr<mpf_class> fa_, fb_;
public:
    ReqPack();
    ReqPack(int id);
    ReqPack(int id, int n1, int n2);
    ReqPack(int id, std::shared_ptr<mpz_class> a, std::shared_ptr<mpz_class> b);
    ReqPack(int id, std::shared_ptr<mpf_class> fa);
    ReqPack(int id, std::shared_ptr<mpf_class> fa, std::shared_ptr<mpf_class> fb);
    ReqPack(int id, std::shared_ptr<mpz_class> a, std::shared_ptr<mpz_class> b, std::shared_ptr<mpz_class> c, std::shared_ptr<mpz_class> d);
    int GetID();
    int GetN1();
    int GetN2();
    PackType GetType();
    std::shared_ptr<mpz_class> Geta();
    std::shared_ptr<mpz_class> Getb();
    std::shared_ptr<mpz_class> Getc();
    std::shared_ptr<mpz_class> Getd();
    std::shared_ptr<mpf_class> Getfa();
    std::shared_ptr<mpf_class> Getfb();
    void Invalidate();
    bool IsValid();
};

class RespPack {
    int id_;
    int n1_;
    int n2_;
    std::shared_ptr<PQT> result_;
    PackType type_;
    std::shared_ptr<mpz_class> a_;
    std::shared_ptr<mpf_class> fa_;
public:
    RespPack();
    RespPack(int id, int n1, int n2, std::shared_ptr<PQT> result);
    RespPack(int id, std::shared_ptr<PQT> result);
    RespPack(ReqPack& rp, std::shared_ptr<PQT> result);
    RespPack(int id, std::shared_ptr<mpz_class> a);
    RespPack(ReqPack& rp, std::shared_ptr<mpz_class> a);
    RespPack(ReqPack& rp, std::shared_ptr<mpf_class> fa);
    int GetID();
    int GetN1();
    int GetN2();
    std::shared_ptr<PQT> GetResult();
    PackType GetType();
    std::shared_ptr<mpz_class> Geta();
    std::shared_ptr<mpf_class> Getfa();
    void Invalidate();
    bool IsValid();
};



void ClockStart();
void ClockEnd(int ms);
void SetCpuAffinity(int cpu_no, std::thread& thread_obj);
void SetCpuAffinity(int cpu_no);
uint32_t GetBatchNum(int num_of_cores);
