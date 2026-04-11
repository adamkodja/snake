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
    std::vector<int> d{-2,-1,1,2,3};
    return { "pgcd("+std::to_string(a)+", "+std::to_string(b)+") = ?",
             E(g), intWrongs(g,[g,d]{return g+pick(d);}) };
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
    return { std::to_string(b)+u8"\u02e3 = "+std::to_string(val)+u8" \u2192 x=?",
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
    {Difficulty::HARD, {}},   // à compléter
};
