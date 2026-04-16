// ─────────────────────────────────────────────────────────────────────────────
//  problems.cpp — Implémentation des générateurs de problèmes
//
//  Ajouter un problème :
//    1. Écrire une fonction static Problem mon_probleme() ci-dessous
//    2. L'insérer dans GENERATORS au niveau voulu (bas du fichier)
// ─────────────────────────────────────────────────────────────────────────────

#include "problems.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <set>

// ── Helpers internes ─────────────────────────────────────────────────────────

// Génère n mauvaises réponses entières distinctes de sol
static std::vector<MathExpr>
intWrongs(int sol, std::function<int()> gen,
          int n = NUM_WRONGS, bool positive = true)
{
    std::set<int>         seen{sol};
    std::vector<MathExpr> result;
    int tries = 400;
    while ((int)result.size() < n && tries-- > 0) {
        int v = gen();
        if (seen.count(v))       continue;
        if (positive && v <= 0)  continue;
        seen.insert(v);
        result.push_back(E(v));
    }
    return result;
}

// Convertit un entier en chiffres indice Unicode  (ex: 10 → "₁₀")
static std::string toSubscript(int n) {
    static const char* const sub[] = {
        u8"\u2080", u8"\u2081", u8"\u2082", u8"\u2083", u8"\u2084",
        u8"\u2085", u8"\u2086", u8"\u2087", u8"\u2088", u8"\u2089"
    };
    std::string result;
    for (char c : std::to_string(n))
        result += sub[c - '0'];
    return result;
}

// Formate un terme signé "+ b" / "- b" pour construire les énoncés
static std::string signedTerm(int x) {
    return (x >= 0 ? " + " : " - ") + std::to_string(std::abs(x));
}

// Génère des mauvaises réponses symboliques distinctes à partir d'un pool
static std::vector<MathExpr>
symbolicWrongs(const std::string& sol,
               const std::vector<std::string>& pool,
               int n = NUM_WRONGS)
{
    std::vector<std::string> cand;
    for (auto& v : pool) if (v != sol) cand.push_back(v);
    std::shuffle(cand.begin(), cand.end(), getRng());
    if ((int)cand.size() > n) cand.resize(n);

    std::vector<MathExpr> out;
    for (auto& v : cand) out.push_back({v, v});
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Niveau EASY
// ─────────────────────────────────────────────────────────────────────────────

static Problem addition_problem() {
    int a = randint(1,20), b = randint(1,20), s = a+b;
    return { std::to_string(a)+" + "+std::to_string(b)+" = ?",
             E(s), intWrongs(s,[s]{return s+randint(-6,6);}) };
}

static Problem subtraction_problem() {
    int a = randint(6,30), b = randint(1,a-1), s = a-b;
    return { std::to_string(a)+" - "+std::to_string(b)+" = ?",
             E(s), intWrongs(s,[s]{return s+randint(-5,5);}) };
}

static Problem multiplication_problem() {
    int a = randint(2,9), b = randint(2,9), s = a*b;
    std::vector<int> d{-b,b,-a,a,-2*a,2*b};
    return { std::to_string(a)+u8" \u00d7 "+std::to_string(b)+" = ?",
             E(s), intWrongs(s,[s,d]{return s+pick(d);}) };
}

static Problem division_problem() {
    int d = randint(2,9), q = randint(2,9);
    return { std::to_string(d*q)+u8" \u00f7 "+std::to_string(d)+" = ?",
             E(q), intWrongs(q,[q]{return q+randint(-4,4);}) };
}

static Problem square_problem() {
    int a = randint(2,12), s = a*a;
    std::vector<int> d{-(2*a-1),2*a+1,2*a-1,-(2*a+1)};
    return { std::to_string(a)+u8"\u00b2 = ?",
             E(s), intWrongs(s,[s,d]{return s+pick(d);}) };
}

static Problem modulo_problem() {
    int b = randint(2,7), sol = randint(1,b-1), a = b*randint(2,8)+sol;
    return { std::to_string(a)+" mod "+std::to_string(b)+" = ?",
             E(sol), intWrongs(sol,[b]{return randint(0,b-1);},NUM_WRONGS,false) };
}

static Problem pgcd_problem() {
    int g = randint(2,9), a = g*randint(2,8), b = g*randint(2,8);
    int sol = std::gcd(a, b);
    std::vector<int> d{-2,-1,1,2,3};
    return { "pgcd("+std::to_string(a)+", "+std::to_string(b)+") = ?",
             E(sol), intWrongs(sol,[sol,d]{return sol+pick(d);}) };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Niveau MEDIUM
// ─────────────────────────────────────────────────────────────────────────────

static Problem linear_equation_problem() {
    std::vector<int> av{-4,-3,-2,2,3,4};
    int a = pick(av), sol = randint(-9,9), c = randint(-15,15), b = c-a*sol;
    return { std::to_string(a)+"x"+signedTerm(b)+" = "+std::to_string(c)+u8" \u2192 x=?",
             E(sol), intWrongs(sol,[sol]{return sol+randint(-5,5);},NUM_WRONGS,false) };
}

static Problem linear_equation2_problem() {
    int sol = randint(-8,8);
    std::vector<int> av{-4,-3,-2,2,3,4};
    int a = pick(av);
    std::vector<int> cv;
    for (int v:{-3,-2,-1,1,2,3}) if(v!=a) cv.push_back(v);
    int c = pick(cv), b = randint(-10,10), d = b+(a-c)*sol;
    return { std::to_string(a)+"x"+signedTerm(b)+" = "+std::to_string(c)+"x"+signedTerm(d)+u8" \u2192 x=?",
             E(sol), intWrongs(sol,[sol]{return sol+randint(-5,5);},NUM_WRONGS,false) };
}

static Problem quadratic_roots_problem() {
    std::vector<int> pool;
    for (int i=-7;i<=7;i++) pool.push_back(i);
    std::shuffle(pool.begin(), pool.end(), getRng());
    int r1=pool[0], r2=pool[1];
    if (r1>r2) std::swap(r1,r2);
    int p=-(r1+r2), q=r1*r2;
    return { u8"x\u00b2"+signedTerm(p)+"x"+signedTerm(q)+" = 0  (min)",
             E(r1), intWrongs(r1,[r1]{return r1+randint(-4,4);},NUM_WRONGS,false) };
}

// ── Valeurs exactes trig ──────────────────────────────────────────────────────

static const std::map<std::string,MathExpr> TRIG_EXPRS = {
    {"0",              {"0",             "0"}},
    {"1",              {"1",             "1"}},
    {"-1",             {"-1",            "-1"}},
    {"1/2",            {"1/2",           "1/2"}},
    {"-1/2",           {"-1/2",          "-1/2"}},
    {u8"\u221a2/2",    {u8"\u221a2/2",   u8"\u221a2/2"}},
    {u8"-\u221a2/2",   {u8"-\u221a2/2",  u8"-\u221a2/2"}},
    {u8"\u221a3/2",    {u8"\u221a3/2",   u8"\u221a3/2"}},
    {u8"-\u221a3/2",   {u8"-\u221a3/2",  u8"-\u221a3/2"}},
    {u8"\u221a3/3",    {u8"\u221a3/3",   u8"\u221a3/3"}},
    {u8"-\u221a3/3",   {u8"-\u221a3/3",  u8"-\u221a3/3"}},
    {u8"\u221a3",      {u8"\u221a3",     u8"\u221a3"}},
    {u8"-\u221a3",     {u8"-\u221a3",    u8"-\u221a3"}},
};

struct TrigEntry { std::string fn, angle, sol_key; };

static const std::vector<TrigEntry> TRIG_TABLE = {
    {"sin", "0",           "0"},
    {"sin", u8"\u03c0/6",  "1/2"},
    {"sin", u8"\u03c0/4",  u8"\u221a2/2"},
    {"sin", u8"\u03c0/3",  u8"\u221a3/2"},
    {"sin", u8"\u03c0/2",  "1"},
    {"sin", u8"\u03c0",    "0"},
    {"sin", u8"3\u03c0/2", "-1"},
    {"cos", "0",           "1"},
    {"cos", u8"\u03c0/6",  u8"\u221a3/2"},
    {"cos", u8"\u03c0/4",  u8"\u221a2/2"},
    {"cos", u8"\u03c0/3",  "1/2"},
    {"cos", u8"\u03c0/2",  "0"},
    {"cos", u8"\u03c0",    "-1"},
    {"cos", u8"3\u03c0/2", "0"},
    {"tan", "0",           "0"},
    {"tan", u8"\u03c0/6",  u8"\u221a3/3"},
    {"tan", u8"\u03c0/4",  "1"},
    {"tan", u8"\u03c0/3",  u8"\u221a3"},
};

static std::vector<MathExpr> trigWrongs(const std::string& key) {
    std::vector<MathExpr> pool;
    for (auto& [k,v] : TRIG_EXPRS) if (k!=key) pool.push_back(v);
    std::shuffle(pool.begin(), pool.end(), getRng());
    if ((int)pool.size() > NUM_WRONGS) pool.resize(NUM_WRONGS);
    return pool;
}

static Problem trig_exact_problem() {
    auto& e = pick(TRIG_TABLE);
    auto& s = TRIG_EXPRS.at(e.sol_key);
    return { e.fn+"("+e.angle+") = ?", s, trigWrongs(e.sol_key) };
}

static Problem log_integer_problem() {
    std::vector<int> bv{2,3,5,10};
    int b=pick(bv), n=randint(1,5), val=1;
    for (int i=0;i<n;i++) val*=b;
    return { "log"+toSubscript(b)+"("+std::to_string(val)+") = ?",
             E(n), intWrongs(n,[n]{return n+randint(-3,3);}) };
}

static Problem exp_equation_problem() {
    std::vector<int> bv{2,3,5};
    int b=pick(bv), s=randint(1,5), val=1;
    for (int i=0;i<s;i++) val*=b;
    return { std::to_string(b)+"^x = "+std::to_string(val)+u8" \u2192 x=?",
             E(s), intWrongs(s,[s]{return s+randint(-3,3);}) };
}

static Problem arithmetic_sequence_problem() {
    int u0=randint(-10,10), r=randint(-5,5), n=randint(3,8), s=u0+n*r;
    return { "u0="+std::to_string(u0)+", r="+std::to_string(r)+
             u8" \u2192 u"+std::to_string(n)+"=?",
             E(s), intWrongs(s,[s]{return s+randint(-6,6);},NUM_WRONGS,false) };
}

static Problem geometric_sequence_problem() {
    std::vector<int> u0v{-3,-2,-1,1,2,3}, qv{-2,2,3};
    int u0=pick(u0v), q=pick(qv), n=randint(2,4), s=u0;
    for (int i=0;i<n;i++) s*=q;
    std::vector<int> d{-u0*2,-u0,u0,u0*2};
    return { "u0="+std::to_string(u0)+", q="+std::to_string(q)+
             u8" \u2192 u"+std::to_string(n)+"=?",
             E(s), intWrongs(s,[s,d]{return s+pick(d);},NUM_WRONGS,false) };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Niveau HARD
// ─────────────────────────────────────────────────────────────────────────────

struct DLEntry {
    std::string f;
    // coeffs de x^0, x^1, x^2, x^3 dans le DL en 0
    std::vector<std::string> c;
};

static const std::vector<DLEntry> DL_TABLE = {
    {"exp(x)",        {"1", "1", "1/2",  "1/6"}},
    {"sin(x)",        {"0", "1", "0",    "-1/6"}},
    {"cos(x)",        {"1", "0", "-1/2", "0"}},
    {"(1+x)^2",       {"1", "2", "1",    "0"}},
    {"ln(1+x)",       {"0", "1", "-1/2", "1/3"}},
    {"1/(1+x)",       {"1", "-1", "1",   "-1"}},
    {"sqrt(1+x)",     {"1", "1/2", "-1/8", "1/16"}},
};

static Problem dl_coefficient_problem() {
    auto& e = pick(DL_TABLE);
    int k = randint(1,3);
    std::string sol = e.c[k];
    std::vector<std::string> pool{
        "0","1","-1","1/2","-1/2","1/3","-1/3","1/6","-1/6","1/8","-1/8","1/16","-1/16"
    };

    return {
        "DL"+toSubscript(3)+" en 0 de "+e.f+" : coeff de x^"+std::to_string(k)+" ?",
        {sol, sol},
        symbolicWrongs(sol, pool)
    };
}

struct EqEntry {
    std::string expr;
    std::string eqv;
};

static const std::vector<EqEntry> EQ_TABLE = {
    {"sin(x)", "x"},
    {"tan(x)", "x"},
    {"1-cos(x)", "x^2/2"},
    {"ln(1+x)", "x"},
    {"exp(x)-1", "x"},
    {"sqrt(1+x)-1", "x/2"},
};

static Problem equivalent_problem() {
    auto& e = pick(EQ_TABLE);
    std::vector<std::string> pool{"x","x/2","x^2","x^2/2","1","-x","-x^2/2"};
    return {
        "x->0 : "+e.expr+" ~ ?",
        {e.eqv, e.eqv},
        symbolicWrongs(e.eqv, pool)
    };
}

static Problem roots_of_unity_order_problem() {
    int n = randint(4,12);
    int k = randint(1,n-1);
    int sol = n / std::gcd(n, k);
    return {
        "z=exp(2i"+std::to_string(k)+"pi/"+std::to_string(n)+") : ordre de z ?",
        E(sol),
        intWrongs(sol, [n]{ return randint(1,n); })
    };
}

static Problem common_roots_problem() {
    int n = randint(4,16), m = randint(4,16);
    int sol = std::gcd(n, m);
    return {
        "Nb racines communes de X^"+std::to_string(n)+"-1 et X^"+std::to_string(m)+"-1 ?",
        E(sol),
        intWrongs(sol, [n,m]{ return randint(1,std::max(n,m)); })
    };
}

struct SeqLimitEntry {
    std::string un;
    std::string lim;
};

static const std::vector<SeqLimitEntry> SEQ_LIMIT_TABLE = {
    {"(2n+1)/(n+3)", "2"},
    {"(3n^2-1)/(n^2+4)", "3"},
    {"ln(n)/n", "0"},
    {"n/(n^2+1)", "0"},
    {"sin(1/n)", "0"},
    {"(1+1/n)^n", "e"},
    {"sqrt(n^2+3n)-n", "3/2"},
};

static Problem convergent_sequence_limit_problem() {
    auto& e = pick(SEQ_LIMIT_TABLE);
    std::vector<std::string> pool{
        "0","1","2","3","-1","1/2","3/2","e"
    };
    return {
        "n->inf : u_n="+e.un+" ; limite ?",
        {e.lim, e.lim},
        symbolicWrongs(e.lim, pool)
    };
}

struct SeriesNatureEntry {
    std::string un;
    std::string nature;
};

static const std::vector<SeriesNatureEntry> SERIES_NATURE_TABLE = {
    {"1/n^2", "absolue"},
    {"(-1)^(n+1)/n", "simple"},
    {"(-1)^n/sqrt(n)", "simple"},
    {"sin(n)/n", "simple"},
    {"1/n", "divergente"},
    {"1/sqrt(n)", "divergente"},
    {"(-1)^n", "divergente"},
};

static Problem series_nature_problem() {
    auto& e = pick(SERIES_NATURE_TABLE);
    std::vector<std::string> pool{"abs", "sim", "div", "?"};
    return {
        "Nature de somme u_n, u_n="+e.un+" ? (absolue/simple/divergente)",
        {e.nature, e.nature},
        symbolicWrongs(e.nature, pool)
    };
}

struct RadiusEntry {
    std::string series;
    std::string radius;
};

static const std::vector<RadiusEntry> POWER_SERIES_TABLE = {
    {"somme x^n", "1"},
    {"somme x^n/n!", "inf"},
    {"somme n! x^n", "0"},
    {"somme n x^n", "1"},
    {"somme x^n/n^2", "1"},
    {"somme (x/3)^n", "3"},
    {"somme (2x)^n", "1/2"},
    {"somme x^(2n)", "1"},
};

static Problem power_series_radius_problem() {
    auto& e = pick(POWER_SERIES_TABLE);
    std::vector<std::string> pool{"0","1/2","1","2","3","inf"};
    return {
        "Rayon de convergence de "+e.series+" ?",
        {e.radius, e.radius},
        symbolicWrongs(e.radius, pool)
    };
}


// ─────────────────────────────────────────────────────────────────────────────
//  Table des générateurs par niveau
//  ► Ajoutez vos fonctions ci-dessus, puis insérez-les ici
// ─────────────────────────────────────────────────────────────────────────────

const std::map<Difficulty, std::vector<ProbGen>> GENERATORS = {
    {Difficulty::EASY, {
        addition_problem,
        subtraction_problem,
        multiplication_problem,
        division_problem,
        square_problem,
        modulo_problem,
        pgcd_problem,
    }},
    {Difficulty::MEDIUM, {
        linear_equation_problem,
        linear_equation2_problem,
        quadratic_roots_problem,
        trig_exact_problem,
        log_integer_problem,
        exp_equation_problem,
        arithmetic_sequence_problem,
        geometric_sequence_problem,
    }},
    {Difficulty::HARD, {
        dl_coefficient_problem,
        equivalent_problem,
        roots_of_unity_order_problem,
        common_roots_problem,
        convergent_sequence_limit_problem,
        series_nature_problem,
        power_series_radius_problem,
    }},
};
