// ─────────────────────────────────────────────────────────────────────────────
//  Snake Maths — C++/SFML port
//  Requires: SFML 2.6  (see CMakeLists.txt)
//  Source encoding: UTF-8
// ─────────────────────────────────────────────────────────────────────────────

#include <SFML/Graphics.hpp>

#include "problems.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────────────────────────────────────

constexpr int   COLS          = 16;
constexpr int   ROWS          = 12;
constexpr float HUD_ROWS_F    = 2.8f;
constexpr int   SPEED_MS      = 180;
constexpr int   LIVES_START   = 3;
constexpr int   NUM_FRUITS    = 4;
constexpr float LIFE_FRUIT_P  = 0.35f;
constexpr int   DEFAULT_CELL  = 36;
constexpr int   SPRITE_FRAMES = 6;
constexpr int   FPS_LIMIT     = 60;

// Head/tail asset faces Right (0°). PIL rotated CCW → SFML rotates CW.
const std::map<std::string, float> SPRITE_ROT = {
    {"Right",   0.f},
    {"Up",    270.f},
    {"Left",  180.f},
    {"Down",   90.f},
};
const std::map<std::string, std::pair<int,int>> DIRS = {
    {"Up",   {0,-1}}, {"Down", {0,1}},
    {"Left", {-1,0}}, {"Right",{1,0}},
};
const std::map<std::string,std::string> OPPOSITE = {
    {"Up","Down"},{"Down","Up"},{"Left","Right"},{"Right","Left"}
};

// ─────────────────────────────────────────────────────────────────────────────
//  Colours
// ─────────────────────────────────────────────────────────────────────────────

const sf::Color C_BG      {0x1a,0x1a,0x2e};
const sf::Color C_GRID    {0x16,0x21,0x3e};
const sf::Color C_HUD     {0x0f,0x34,0x60};
const sf::Color C_SNAKE_H {0xe9,0x45,0x60};
const sf::Color C_SNAKE_B {0x53,0x60,0x7a};
const sf::Color C_FRUIT_O {0x4e,0xcc,0xa3};
const sf::Color C_TXT     {0xea,0xea,0xea};
const sf::Color C_LIVES   {0xe9,0x45,0x60};
const sf::Color C_OVERLAY {0x0f,0x34,0x60};
const sf::Color C_ACCENT  {0xe9,0x45,0x60};

// ─────────────────────────────────────────────────────────────────────────────
//  Fruit
// ─────────────────────────────────────────────────────────────────────────────

using Pos = std::pair<int,int>;

struct Fruit {
    int      col, row;
    MathExpr value;
    bool     correct;
    bool     is_life = false;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Text helper (wraps sf::Text + UTF-8 → sf::String conversion)
// ─────────────────────────────────────────────────────────────────────────────

// drawText alignment
enum class HA { Left, Center, Right };
enum class VA { Top,  Center, Bottom };

// ─────────────────────────────────────────────────────────────────────────────
//  Main game class
// ─────────────────────────────────────────────────────────────────────────────

class SnakeMathGame {
public:
    SnakeMathGame();
    void run();

private:
    // ── Window & resources ──────────────────────────────────────────────────
    sf::RenderWindow window_;
    sf::Font         font_;
    bool             fontOk_ = false;

    std::vector<sf::Texture> headTex_{SPRITE_FRAMES};
    std::vector<sf::Texture> bodyTex_{SPRITE_FRAMES};
    std::vector<sf::Texture> tailTex_{SPRITE_FRAMES};
    bool spritesOk_ = false;

    // ── Dynamic sizing ──────────────────────────────────────────────────────
    float cell_  = (float)DEFAULT_CELL;
    float hudH_  = (float)DEFAULT_CELL * HUD_ROWS_F;

    // ── Game state ──────────────────────────────────────────────────────────
    std::vector<Pos>       snake_, prevSnake_;
    std::string            dir_  = "Right";
    std::string            ndir_ = "Right";
    int                    lives_= LIVES_START;
    int                    score_= 0;
    bool                   over_ = false;
    bool                   paused_= false;
    std::vector<Fruit>     fruits_;
    std::optional<Problem> prob_;
    int                    aframe_= 0;

    Difficulty   diff_       = Difficulty::EASY;
    bool         menuOpen_   = true;
    bool         started_    = false;

    // ── Timing ──────────────────────────────────────────────────────────────
    sf::Clock clock_;
    float     stepAccum_ = 0.f;   // accumulated ms since last step
    float     lastStep_  = 0.f;   // ms of last step (for interpolation)

    // ── Menu click zones ────────────────────────────────────────────────────
    struct Btn { sf::FloatRect rect; std::string action; };
    std::vector<Btn> btns_;

    // ── Methods ─────────────────────────────────────────────────────────────
    void loadResources();
    void onResize(unsigned w, unsigned h);
    void initGame();
    void newProblem();
    void placeFruits();
    Pos  freeCell(const std::set<Pos>& occ);
    void step();
    void loseLife();
    std::string tailDir() const;

    void handleEvents();
    void render(float t);

    void drawGrid();
    void drawHUD();
    void drawFruits();
    void drawSnake(float t);
    void drawMenu();
    void drawOverlay(const std::string& title, const std::string& sub);

    // Text helpers
    void drawText(const std::string& str, float x, float y, unsigned sz,
                  sf::Color col, bool bold=false,
                  HA ha=HA::Center, VA va=VA::Center);
    sf::FloatRect measureText(const std::string& str, unsigned sz, bool bold=false);

    // Sprite helper
    void drawSprite(sf::Texture& tex, float cx, float cy, float size, float deg=0.f);
};

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

SnakeMathGame::SnakeMathGame()
    : window_(sf::VideoMode(COLS*DEFAULT_CELL,
                            (unsigned)((ROWS+HUD_ROWS_F)*DEFAULT_CELL)),
              "Snake Maths",
              sf::Style::Default)
{
    window_.setFramerateLimit(FPS_LIMIT);
    window_.setKeyRepeatEnabled(false);
    loadResources();
    initGame();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resources
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::loadResources() {
    // Font — try several common locations
    for (auto& p : std::vector<std::string>{
            "assets/font.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/calibri.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/System/Library/Fonts/Helvetica.ttc"})
    {
        if (font_.loadFromFile(p)) { fontOk_=true; break; }
    }

    // Snake sprites
    bool ok = true;
    fs::path assets{"assets"};
    for (int f=0; f<SPRITE_FRAMES; f++) {
        auto n = std::to_string(f);
        if (!headTex_[f].loadFromFile((assets/("snake_neo_arcade_head_"+n+".png")).string())) ok=false;
        if (!bodyTex_[f].loadFromFile((assets/("snake_neo_arcade_body_"+n+".png")).string())) ok=false;
        if (!tailTex_[f].loadFromFile((assets/("snake_neo_arcade_tail_"+n+".png")).string())) ok=false;
    }
    if (ok) {
        for (auto& t:headTex_) t.setSmooth(true);
        for (auto& t:bodyTex_) t.setSmooth(true);
        for (auto& t:tailTex_) t.setSmooth(true);
        spritesOk_ = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resize
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::onResize(unsigned w, unsigned h) {
    window_.setView(sf::View(sf::FloatRect(0,0,(float)w,(float)h)));
    cell_ = std::min((float)w / COLS, (float)h / (ROWS + HUD_ROWS_F));
    hudH_ = cell_ * HUD_ROWS_F;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game initialisation
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::initGame() {
    int cx=COLS/2, cy=ROWS/2;
    snake_    = {{cx,cy},{cx-1,cy},{cx-2,cy}};
    prevSnake_= snake_;
    dir_ = ndir_ = "Right";
    lives_= LIVES_START;
    score_= 0;
    over_ = paused_ = false;
    fruits_.clear();
    prob_.reset();
    aframe_  = 0;
    stepAccum_= 0.f;
    newProblem();
}

void SnakeMathGame::newProblem() {
    auto& gens = GENERATORS.at(diff_);
    prob_ = pick(gens)();
    placeFruits();
}

void SnakeMathGame::placeFruits() {
    std::set<Pos> occ(snake_.begin(), snake_.end());

    std::vector<MathExpr> answers{prob_->solution};
    for (auto& w:prob_->wrong_answers) answers.push_back(w);

    // Pad with extra distractors if needed
    while ((int)answers.size() < NUM_FRUITS) {
        int extra = 0;
        if (!answers.empty() && !answers[0].value.empty()) {
            try { extra = std::stoi(answers[0].value) + randint(1,10); }
            catch (...) { extra = randint(1,20); }
        } else { extra = randint(1,20); }
        answers.push_back(E(extra));
    }
    answers.resize(NUM_FRUITS);
    std::shuffle(answers.begin(), answers.end(), getRng());

    fruits_.clear();
    for (auto& val:answers) {
        auto [c,r] = freeCell(occ);
        occ.insert({c,r});
        fruits_.push_back({c, r, val, val==prob_->solution});
    }
    // Life fruit
    if (lives_ < LIVES_START && randf() < LIFE_FRUIT_P) {
        auto [c,r] = freeCell(occ);
        fruits_.push_back({c, r, {"life", u8"\u2665"}, false, true});
    }
}

Pos SnakeMathGame::freeCell(const std::set<Pos>& occ) {
    while (true) {
        Pos p{randint(0,COLS-1), randint(0,ROWS-1)};
        if (!occ.count(p)) return p;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game step
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::step() {
    dir_ = ndir_;
    auto [dx,dy] = DIRS.at(dir_);
    auto [hx,hy] = snake_.front();
    int nx=hx+dx, ny=hy+dy;

    if (nx<0||nx>=COLS||ny<0||ny>=ROWS) { loseLife(); return; }

    std::set<Pos> body(snake_.begin(), snake_.end());
    if (body.count({nx,ny}))             { loseLife(); return; }

    Fruit* eaten = nullptr;
    for (auto& f:fruits_) if(f.col==nx && f.row==ny) { eaten=&f; break; }

    snake_.insert(snake_.begin(), {nx,ny});

    if (!eaten) {
        snake_.pop_back();
    } else if (eaten->is_life) {
        snake_.pop_back();
        lives_ = std::min(LIVES_START, lives_+1);
        fruits_.erase(std::remove_if(fruits_.begin(),fruits_.end(),
            [eaten](const Fruit& f){ return &f==eaten; }), fruits_.end());
    } else if (eaten->correct) {
        score_++;
        newProblem();                 // grow (no pop_back)
    } else {
        snake_.pop_back();
        lives_--;
        fruits_.erase(std::remove_if(fruits_.begin(),fruits_.end(),
            [eaten](const Fruit& f){ return &f==eaten; }), fruits_.end());
        if (lives_<=0) over_=true;
    }
}

void SnakeMathGame::loseLife() {
    lives_--;
    if (lives_<=0) { over_=true; return; }
    int cx=COLS/2, cy=ROWS/2;
    snake_ = {{cx,cy},{cx-1,cy},{cx-2,cy}};
    prevSnake_ = snake_;
    dir_ = ndir_ = "Right";
}

std::string SnakeMathGame::tailDir() const {
    if (snake_.size()<2) return dir_;
    auto [tx,ty]=snake_.back();
    auto [px,py]=snake_[snake_.size()-2];
    int dx=tx-px, dy=ty-py;
    if (dx>0) return "Right"; if (dx<0) return "Left";
    return (dy>0) ? "Down" : "Up";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Run loop
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::run() {
    clock_.restart();
    while (window_.isOpen()) {
        float dt = static_cast<float>(clock_.restart().asMilliseconds());

        handleEvents();

        if (!over_ && !paused_ && !menuOpen_) {
            stepAccum_ += dt;
            if (stepAccum_ >= SPEED_MS) {
                prevSnake_ = snake_;
                aframe_    = (aframe_+1) % SPRITE_FRAMES;
                step();
                lastStep_  = SPEED_MS;
                stepAccum_ -= SPEED_MS;
            }
            lastStep_ = (float)SPEED_MS;
        }

        float t = (lastStep_ > 0.f)
                  ? std::min(1.f, (SPEED_MS - stepAccum_) / lastStep_)
                  : 1.f;
        // t=1 → fully at new position, t=0 → still at previous
        // We want t to grow from 0→1 between two steps
        t = 1.f - t;      // remap: 0 at step boundary, 1 when next step due

        render(t);
    }
}

void SnakeMathGame::handleEvents() {
    sf::Event ev;
    while (window_.pollEvent(ev)) {
        switch (ev.type) {
        case sf::Event::Closed:
            window_.close(); break;

        case sf::Event::Resized:
            onResize(ev.size.width, ev.size.height); break;

        case sf::Event::KeyPressed:
            switch (ev.key.code) {
            case sf::Keyboard::Escape:
                menuOpen_ = !menuOpen_;
                paused_   =  menuOpen_;
                break;
            case sf::Keyboard::P:
                if (!menuOpen_) paused_ = !paused_;
                break;
            case sf::Keyboard::R:
                if (over_) initGame();
                break;
            case sf::Keyboard::Up:
                if (!menuOpen_ && ndir_!="Down")  ndir_="Up";   break;
            case sf::Keyboard::Down:
                if (!menuOpen_ && ndir_!="Up")    ndir_="Down"; break;
            case sf::Keyboard::Left:
                if (!menuOpen_ && ndir_!="Right") ndir_="Left"; break;
            case sf::Keyboard::Right:
                if (!menuOpen_ && ndir_!="Left")  ndir_="Right";break;
            default: break;
            }
            break;

        case sf::Event::MouseButtonPressed:
            if (!menuOpen_) break;
            for (auto& b:btns_) {
                if (b.rect.contains((float)ev.mouseButton.x,(float)ev.mouseButton.y)) {
                    // Actions
                    if (b.action=="resume") {
                        started_=true; menuOpen_=false; paused_=false;
                    } else if (b.action=="quit") {
                        window_.close();
                    } else if (b.action=="EASY") {
                        diff_=Difficulty::EASY;   initGame(); started_=true; menuOpen_=false;
                    } else if (b.action=="MEDIUM") {
                        diff_=Difficulty::MEDIUM; initGame(); started_=true; menuOpen_=false;
                    } else if (b.action=="HARD") {
                        // no generators — do nothing
                    }
                    break;
                }
            }
            break;

        default: break;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Rendering
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::render(float t) {
    window_.clear(C_BG);
    drawGrid();
    drawFruits();
    drawSnake(t);
    drawHUD();

    if (menuOpen_)       drawMenu();
    else if (paused_)    drawOverlay("PAUSE", "P  pour continuer");
    if (over_)           drawOverlay("GAME OVER",
                             "Score : "+std::to_string(score_)+"   —   R pour rejouer");

    window_.display();
}

// ── Grid ─────────────────────────────────────────────────────────────────────

void SnakeMathGame::drawGrid() {
    sf::RectangleShape cell({cell_-1.f, cell_-1.f});
    cell.setOutlineColor(C_BG);
    cell.setOutlineThickness(0.5f);
    for (int r=0;r<ROWS;r++) for (int c=0;c<COLS;c++) {
        cell.setPosition(c*cell_, hudH_+r*cell_);
        cell.setFillColor(C_GRID);
        window_.draw(cell);
    }
}

// ── HUD ──────────────────────────────────────────────────────────────────────

void SnakeMathGame::drawHUD() {
    float cw = cell_*COLS;

    sf::RectangleShape hud({cw, hudH_});
    hud.setFillColor(C_HUD);
    window_.draw(hud);

    // Problem statement
    if (prob_) {
        unsigned fs = std::max(10u,(unsigned)(cell_*0.52f));
        drawText(prob_->statement, cw/2.f, hudH_*0.35f, fs, C_TXT, true);
    }

    // Hearts (individual so we can colour each differently)
    unsigned hSz = std::max(10u, (unsigned)(cell_*0.42f));
    float hx = cell_*0.3f, hy = hudH_*0.75f;
    for (int i=0;i<LIVES_START;i++) {
        sf::Color col = (i<lives_) ? C_LIVES : sf::Color(0x44,0x33,0x44);
        auto b = measureText(u8"\u2665", hSz);
        drawText(u8"\u2665", hx + b.width/2.f, hy, hSz, col, false, HA::Center, VA::Center);
        hx += b.width + cell_*0.12f;
    }

    // Score (right-aligned)
    std::string sc = "Score : "+std::to_string(score_);
    unsigned sfs = std::max(10u,(unsigned)(cell_*0.40f));
    drawText(sc, cw - cell_*0.3f, hudH_*0.75f, sfs, C_TXT, false, HA::Right, VA::Center);
}

// ── Fruits ───────────────────────────────────────────────────────────────────

void SnakeMathGame::drawFruits() {
    for (auto& fr:fruits_) {
        float pad = std::max(2.f, cell_*0.06f);
        float x0=fr.col*cell_+pad,      y0=fr.row*cell_+hudH_+pad;
        float x1=fr.col*cell_+cell_-pad, y1=fr.row*cell_+hudH_+cell_-pad;
        float cx=(x0+x1)/2.f, cy=(y0+y1)/2.f;

        sf::RectangleShape box({x1-x0,y1-y0});
        box.setPosition(x0,y0);
        box.setOutlineThickness(std::max(1.f,cell_*0.05f));

        if (fr.is_life) {
            box.setFillColor({0x3a,0x0a,0x18});
            box.setOutlineColor(C_LIVES);
            window_.draw(box);
            drawText(u8"\u2665", cx, cy, (unsigned)(cell_*0.50f), C_LIVES);
        } else {
            box.setFillColor({0x0f,0x1e,0x3c});
            box.setOutlineColor(C_FRUIT_O);
            window_.draw(box);
            unsigned fs = std::max(8u,(unsigned)(cell_*0.34f));
            drawText(fr.value.display, cx, cy, fs, C_TXT);
        }
    }
}

// ── Snake ─────────────────────────────────────────────────────────────────────

void SnakeMathGame::drawSnake(float t) {
    int n = (int)snake_.size();
    float size = cell_*0.92f;

    for (int i=0;i<n;i++) {
        auto [cx,cy]   = snake_[i];
        float px_curr  = cx*cell_+cell_/2.f;
        float py_curr  = cy*cell_+hudH_+cell_/2.f;
        float px_prev  = px_curr, py_prev = py_curr;
        if (i < (int)prevSnake_.size()) {
            auto [pc,pr] = prevSnake_[i];
            px_prev = pc*cell_+cell_/2.f;
            py_prev = pr*cell_+hudH_+cell_/2.f;
        }
        float px = px_prev + (px_curr-px_prev)*t;
        float py = py_prev + (py_curr-py_prev)*t;

        if (spritesOk_) {
            int f = aframe_ % SPRITE_FRAMES;
            if (i==0) {
                drawSprite(headTex_[f], px, py, size, SPRITE_ROT.at(dir_));
            } else if (i==n-1) {
                drawSprite(tailTex_[f], px, py, size, SPRITE_ROT.at(tailDir()));
            } else {
                drawSprite(bodyTex_[(aframe_+i)%SPRITE_FRAMES], px, py, size);
            }
        } else {
            float m = cell_*0.06f;
            sf::RectangleShape seg({size,size});
            seg.setPosition(px-size/2.f, py-size/2.f);
            seg.setFillColor(i==0 ? C_SNAKE_H : C_SNAKE_B);
            window_.draw(seg);
        }
    }
}

// ── Menu ─────────────────────────────────────────────────────────────────────

void SnakeMathGame::drawMenu() {
    auto sz   = window_.getSize();
    float W   = (float)sz.x, H = (float)sz.y;

    float btnH  = std::max(24.f, cell_*0.85f);
    float btnW  = std::max(150.f, cell_*6.8f);
    float gap   = std::max(6.f,   cell_*0.22f);
    float gapSm = gap*0.45f;
    float titH  = std::max(18.f,  cell_*0.90f);
    float sepH  = std::max(12.f,  cell_*0.55f);
    float padV  = std::max(12.f,  cell_*0.55f);
    float padH  = std::max(14.f,  cell_*0.60f);

    float totalH = padV+titH+gap + btnH+gap + sepH+gapSm
                   + btnH+gapSm+btnH+gapSm+btnH + gap + btnH+padV;
    float bw = btnW+2.f*padH;
    float bx0=(W-bw)/2.f, by0=(H-totalH)/2.f;
    float btnX0=bx0+padH, btnX1=bx0+bw-padH;

    // Background veil
    sf::RectangleShape veil({W,H});
    veil.setFillColor({0x0a,0x0a,0x1a,0x99});
    window_.draw(veil);

    // Box
    sf::RectangleShape box({bw,totalH});
    box.setPosition(bx0,by0);
    box.setFillColor(C_OVERLAY);
    box.setOutlineColor(C_ACCENT);
    box.setOutlineThickness(std::max(1.f,cell_*0.07f));
    window_.draw(box);

    btns_.clear();
    float cur = by0+padV;

    // Title
    drawText("MENU", W/2.f, cur+titH/2.f,
             (unsigned)std::max(12.f,cell_*0.65f), C_ACCENT, true);
    cur += titH+gap;

    // Helper lambda
    auto drawBtn = [&](const std::string& lbl, const std::string& action,
                       bool active, bool disabled, float gAfter) {
        float y0=cur, y1=cur+btnH;
        sf::Color fill, outline, tc;
        if (disabled) { fill={0x25,0x25,0x38}; outline={0x44,0x44,0x5a}; tc={0x55,0x55,0x6a}; }
        else if (active)  { fill=C_ACCENT; outline=C_TXT; tc=C_BG; }
        else { fill=C_HUD; outline=C_ACCENT; tc=C_TXT; }

        sf::RectangleShape btn({btnX1-btnX0, btnH});
        btn.setPosition(btnX0,y0);
        btn.setFillColor(fill);
        btn.setOutlineColor(outline);
        btn.setOutlineThickness(std::max(1.f,cell_*0.05f));
        window_.draw(btn);

        drawText(lbl, (btnX0+btnX1)/2.f, (y0+y1)/2.f,
                 (unsigned)std::max(9.f,cell_*0.36f), tc, true);

        if (!disabled)
            btns_.push_back({{btnX0,y0,btnX1-btnX0,btnH}, action});
        cur = y1+gAfter;
    };

    // Start / Resume
    drawBtn(started_ ? u8"\u25b6 Reprendre" : u8"\u25b6 Commencer",
            "resume", false, false, gap);

    // Difficulty label
    drawText("Difficulte", W/2.f, cur+sepH/2.f,
             (unsigned)std::max(8.f,cell_*0.30f), C_TXT);
    cur += sepH+gapSm;

    // Difficulty buttons
    struct DiffBtn { Difficulty d; std::string lbl; std::string act; };
    std::vector<DiffBtn> diffs = {
        {Difficulty::EASY,   "Facile",             "EASY"},
        {Difficulty::MEDIUM, "Moyen",              "MEDIUM"},
        {Difficulty::HARD,   "Difficile (bientot)","HARD"},
    };
    for (int i=0;i<(int)diffs.size();i++) {
        auto& db = diffs[i];
        bool empty    = GENERATORS.at(db.d).empty();
        bool isLast   = (i==(int)diffs.size()-1);
        drawBtn(db.lbl, db.act,
                db.d==diff_ && !empty, empty,
                isLast ? gap : gapSm);
    }

    // Quit
    drawBtn(u8"\u2715 Quitter", "quit", false, false, padV);
}

// ── Overlay ───────────────────────────────────────────────────────────────────

void SnakeMathGame::drawOverlay(const std::string& title, const std::string& sub) {
    auto sz = window_.getSize();
    float W=(float)sz.x, H=(float)sz.y;
    float bx0=W/5.f, by0=H/3.f, bx1=4.f*W/5.f, by1=2.f*H/3.f;

    sf::RectangleShape box({bx1-bx0,by1-by0});
    box.setPosition(bx0,by0);
    box.setFillColor(C_OVERLAY);
    box.setOutlineColor(C_ACCENT);
    box.setOutlineThickness(std::max(1.f,cell_*0.08f));
    window_.draw(box);

    float cx=(bx0+bx1)/2.f, cy=(by0+by1)/2.f;
    drawText(title, cx, cy-cell_*0.7f,
             (unsigned)std::max(12u,(unsigned)(cell_*0.75f)), C_ACCENT, true);
    drawText(sub,   cx, cy+cell_*0.5f,
             (unsigned)std::max(8u, (unsigned)(cell_*0.36f)), C_TXT);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Text helpers
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::drawText(const std::string& str, float x, float y,
                              unsigned sz, sf::Color col, bool bold,
                              HA ha, VA va)
{
    if (!fontOk_) return;
    sf::Text text;
    text.setFont(font_);
    text.setString(sf::String::fromUtf8(str.begin(), str.end()));
    text.setCharacterSize(sz);
    text.setFillColor(col);
    if (bold) text.setStyle(sf::Text::Bold);

    auto b = text.getLocalBounds();
    float ox = (ha==HA::Left) ? b.left
             : (ha==HA::Right)? b.left+b.width
             :                  b.left+b.width/2.f;
    float oy = (va==VA::Top)    ? b.top
             : (va==VA::Bottom) ? b.top+b.height
             :                    b.top+b.height/2.f;
    text.setOrigin(ox, oy);
    text.setPosition(x, y);
    window_.draw(text);
}

sf::FloatRect SnakeMathGame::measureText(const std::string& str, unsigned sz, bool bold) {
    if (!fontOk_) return {};
    sf::Text t; t.setFont(font_);
    t.setString(sf::String::fromUtf8(str.begin(),str.end()));
    t.setCharacterSize(sz);
    if (bold) t.setStyle(sf::Text::Bold);
    return t.getLocalBounds();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sprite helper
// ─────────────────────────────────────────────────────────────────────────────

void SnakeMathGame::drawSprite(sf::Texture& tex, float cx, float cy,
                                float size, float deg)
{
    sf::Sprite spr(tex);
    auto ts = tex.getSize();
    spr.setOrigin(ts.x/2.f, ts.y/2.f);
    spr.setScale(size/ts.x, size/ts.y);
    spr.setRotation(deg);
    spr.setPosition(cx, cy);
    window_.draw(spr);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    SnakeMathGame game;
    game.run();
    return 0;
}
