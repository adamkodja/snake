// ─────────────────────────────────────────────────────────────────────────────
//  problems.hpp — Structures, générateurs de problèmes et rendu mathématique
//
//  Conventions
//  ───────────
//  • MathExpr.value   : clé de comparaison (ex. "12", "√3/2")
//  • MathExpr.display : chaîne UTF-8 affichée dans le jeu
//  • Problem.statement: chaîne UTF-8 de l'énoncé
//
//  Pour ajouter un problème
//  ────────────────────────
//  1. Définir une fonction () -> Problem dans problems.cpp
//  2. L'ajouter dans GENERATORS au niveau voulu
//  NUM_WRONGS doit rester égal à NUM_FRUITS - 1 (défini dans snake_math.cpp)
// ─────────────────────────────────────────────────────────────────────────────

#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <vector>

// ── Constante partagée ────────────────────────────────────────────────────────

constexpr int NUM_WRONGS = 3;   // = NUM_FRUITS - 1 dans snake_math.cpp

// ── Moteur aléatoire partagé ─────────────────────────────────────────────────

inline std::mt19937& getRng() {
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}
inline int randint(int lo, int hi) {
    return std::uniform_int_distribution<int>{lo, hi}(getRng());
}
inline float randf() {
    return std::uniform_real_distribution<float>{0.f, 1.f}(getRng());
}
template<typename T>
inline const T& pick(const std::vector<T>& v) {
    return v[static_cast<std::size_t>(randint(0, (int)v.size() - 1))];
}

// ── Structures de base ────────────────────────────────────────────────────────

struct MathExpr {
    std::string value;    // clé d'égalité
    std::string display;  // texte UTF-8 affiché

    bool operator==(const MathExpr& o) const { return value == o.value; }
    bool operator< (const MathExpr& o) const { return value < o.value;  }
};

struct Problem {
    std::string           statement;
    MathExpr              solution;
    std::vector<MathExpr> wrong_answers;
};

// Entier → MathExpr (inline car utilisé dans snake_math.cpp aussi)
inline MathExpr E(int n) {
    auto s = std::to_string(n);
    return {s, s};
}

// ── Niveaux de difficulté ─────────────────────────────────────────────────────

enum class Difficulty { EASY, MEDIUM, HARD };

using ProbGen = std::function<Problem()>;

// Table des générateurs par niveau — définie dans problems.cpp
extern const std::map<Difficulty, std::vector<ProbGen>> GENERATORS;
