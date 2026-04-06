// (VELIKI KOD - sve popravljeno i stabilno)

#include <iostream>
#include <raylib.h>
#include <deque>
#include <raymath.h>
#include <vector>
#include <fstream>
#include <algorithm>

using namespace std;

// ---------------- GLOBAL ----------------
int velicina_celije = 30;
int broj_celija = 25;
int ofset = 75;
double poslednj_update = 0;

Color zelena = {173,204,96,255};
Color tamno_zelena = {43,51,24,255};

// ---------------- AUDIO ----------------
Music music;
Sound eatSound;
Sound moveSound;

float masterVol = 1.0f;
float musicVol = 0.5f;
float sfxVol = 0.7f;

// ---------------- SCORE ----------------
struct ScoreEntry {
    string name;
    int score;
};

vector<ScoreEntry> scoreboard;

// ---------------- STATE ----------------
enum GameState {
    MENU,
    PLAY,
    SCOREBOARD,
    SETTINGS,
    GAME_OVER,
    PAUSE,
    NAME_INPUT
};

// ---------------- UTILS ----------------
int CenterText(const char* text, int size)
{
    return (GetScreenWidth() - MeasureText(text, size)) / 2;
}

bool eventTriggered(double interval)
{
    double vreme = GetTime();
    if (vreme - poslednj_update >= interval)
    {
        poslednj_update = vreme;
        return true;
    }
    return false;
}

void LoadScores()
{
    ifstream f("scores.txt");
    string n; int s;
    while (f >> n >> s)
        scoreboard.push_back({n,s});
}

void SaveScores()
{
    ofstream f("scores.txt");
    for (auto &e : scoreboard)
        f << e.name << " " << e.score << endl;
}

void SortScores()
{
    sort(scoreboard.begin(), scoreboard.end(),
        [](auto &a, auto &b){ return a.score > b.score; });

    if (scoreboard.size() > 10)
        scoreboard.resize(10);
}

bool IsHighScore(int score)
{
    if (scoreboard.size() < 10) return true;
    return score > scoreboard.back().score;
}

// ---------------- ZMIJA ----------------
class Zmija {
public:
    deque<Vector2> telo = {{6,9},{5,9},{4,9}};
    Vector2 dir = {1,0};
    bool grow = false;

    void Update()
    {
        telo.push_front(Vector2Add(telo[0], dir));
        if (!grow) telo.pop_back();
        else grow = false;
    }

    void Reset()
    {
        telo = {{6,9},{5,9},{4,9}};
        dir = {1,0};
    }

    void Draw()
    {
        for (auto &s : telo)
        {
            DrawRectangleRounded(
                {ofset+s.x*velicina_celije, ofset+s.y*velicina_celije,
                 (float)velicina_celije,(float)velicina_celije},
                0.5,6,tamno_zelena);
        }
    }
};

// ---------------- JABUKA ----------------
class Jabuka {
public:
    Vector2 pos;
    Texture2D tex;

    Jabuka(deque<Vector2> telo)
    {
        Image img = LoadImage("Grafika/jabuka.png");
        ImageResize(&img,30,30);
        tex = LoadTextureFromImage(img);
        UnloadImage(img);

        pos = RandomPos(telo);
    }

    Vector2 RandomCell()
    {
        return {(float)GetRandomValue(0,24),(float)GetRandomValue(0,24)};
    }

    Vector2 RandomPos(deque<Vector2> telo)
    {
        Vector2 p = RandomCell();
        while (find(telo.begin(), telo.end(), p)!=telo.end())
            p = RandomCell();
        return p;
    }

    void Draw()
    {
        DrawTexture(tex, ofset+pos.x*30, ofset+pos.y*30, WHITE);
    }
};

// ---------------- IGRA ----------------
class Igra {
public:
    Zmija zmija;
    Jabuka jabuka = Jabuka(zmija.telo);
    int score = 0;
    bool gameOver = false;

    void Update()
    {
        zmija.Update();

        if (Vector2Equals(zmija.telo[0], jabuka.pos))
        {
            jabuka.pos = jabuka.RandomPos(zmija.telo);
            zmija.grow = true;
            score++;
            PlaySound(eatSound);
        }

        if (zmija.telo[0].x<0 || zmija.telo[0].x>=25 ||
            zmija.telo[0].y<0 || zmija.telo[0].y>=25)
            gameOver = true;
    }

    void Reset()
    {
        zmija.Reset();
        score = 0;
        gameOver = false;
    }

    void Draw()
    {
        zmija.Draw();
        jabuka.Draw();
    }
};

// ---------------- MAIN ----------------
int main()
{
    InitWindow(900,900,"Snake");
    InitAudioDevice();

    music = LoadMusicStream("audio/music.mp3");
    eatSound = LoadSound("audio/eat.wav");
    moveSound = LoadSound("audio/move.wav");

    PlayMusicStream(music);

    LoadScores();

    GameState state = MENU;
    int selected = 0;
    string playerName = "";

    Igra igra;

    SetTargetFPS(60);

   while (!WindowShouldClose())
{
    UpdateMusicStream(music);
    SetMasterVolume(masterVol);
    SetMusicVolume(music, musicVol);
    SetSoundVolume(eatSound, sfxVol);
    SetSoundVolume(moveSound, sfxVol);

    BeginDrawing();
    ClearBackground(zelena);

    // ---------------- MENU ----------------
    if (state == MENU)
    {
        DrawText("RETRO ZMIJA", CenterText("RETRO ZMIJA",60),100,60,tamno_zelena);

        const char* options[4] = {"PLAY","SCOREBOARD","SETTINGS","EXIT"};

        if (IsKeyPressed(KEY_DOWN)) selected++;
        if (IsKeyPressed(KEY_UP)) selected--;
        selected = (selected+4)%4; // wrap around

        float startY = 250;
        float spacing = 70;

        for(int i=0;i<4;i++)
        {
            string text = (i==selected ? "> " : "") + string(options[i]);
            DrawText(text.c_str(), CenterText(options[i],40), startY + i*spacing, 40, tamno_zelena);
        }

        if(IsKeyPressed(KEY_ENTER))
        {
            switch(selected)
            {
                case 0: igra.Reset(); state=PLAY; break;
                case 1: state=SCOREBOARD; break;
                case 2: state=SETTINGS; break;
                case 3: CloseAudioDevice(); CloseWindow(); exit(0); break;
            }
        }
    }

    // ---------------- PLAY ----------------
    if (state == PLAY)
    {
        if (eventTriggered(0.2)) igra.Update();

        // KONTROLE
        if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && igra.zmija.dir.y != 1)
        {
            igra.zmija.dir = {0,-1};
            PlaySound(moveSound);
        }
        if ((IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) && igra.zmija.dir.y != -1)
        {
            igra.zmija.dir = {0,1};
            PlaySound(moveSound);
        }
        if ((IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) && igra.zmija.dir.x != 1)
        {
            igra.zmija.dir = {-1,0};
            PlaySound(moveSound);
        }
        if ((IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) && igra.zmija.dir.x != -1)
        {
            igra.zmija.dir = {1,0};
            PlaySound(moveSound);
        }

        if (IsKeyPressed(KEY_SPACE)) { state = PAUSE; selected=0; }

        DrawRectangleLinesEx({(float)ofset-5,(float)ofset-5,(float)broj_celija*velicina_celije+10,(float)broj_celija*velicina_celije+10}, 5, tamno_zelena);
        DrawText("Retro zmija", ofset-5, 20, 40, tamno_zelena);
        DrawText(TextFormat("%i", igra.score), velicina_celije*broj_celija + 10, 20, 40, tamno_zelena);

        igra.Draw();

        if (igra.gameOver)
        {
            if (IsHighScore(igra.score)) state = NAME_INPUT;
            else state = GAME_OVER;
            selected = 0;
        }
    }

    // ---------------- PAUSE ----------------
    if (state == PAUSE)
    {
        DrawText("PAUSE", CenterText("PAUSE",60),150,60,tamno_zelena);

        const char* options[4] = {"RESUME","SCOREBOARD","SETTINGS","EXIT"};

        if (IsKeyPressed(KEY_DOWN)) selected++;
        if (IsKeyPressed(KEY_UP)) selected--;
        selected = (selected+4)%4;

        float startY = 300;
        float spacing = 70;

        for(int i=0;i<4;i++)
        {
            string text = (i==selected ? "> " : "") + string(options[i]);
            DrawText(text.c_str(), CenterText(options[i],40), startY + i*spacing, 40, tamno_zelena);
        }

        if (IsKeyPressed(KEY_ENTER))
        {
            switch(selected)
            {
                case 0: state = PLAY; break;
                case 1: state = SCOREBOARD; break;
                case 2: state = SETTINGS; break;
                case 3: CloseAudioDevice(); CloseWindow(); exit(0); break;
            }
        }
    }

    // ---------------- GAME OVER ----------------
    if (state == GAME_OVER)
    {
        DrawText("GAME OVER", CenterText("GAME OVER",60),150,60,tamno_zelena);

        const char* options[4]={"RESTART","SCOREBOARD","SETTINGS","EXIT"};

        if (IsKeyPressed(KEY_DOWN)) selected++;
        if (IsKeyPressed(KEY_UP)) selected--;
        selected=(selected+4)%4;

        for (int i=0;i<4;i++)
            DrawText(i==selected?TextFormat("> %s",options[i]):options[i],
                     CenterText(options[i],40),300+i*70,40,tamno_zelena);



        if (IsKeyPressed(KEY_ENTER))
        {
            if (selected==0){ igra.Reset(); state=PLAY; }
            if (selected==1) state=SCOREBOARD;
            if (selected==2) state=SETTINGS;
            if (selected==3) 
            {
                
            }
            selected=0;
        }
    }

    // ---------------- NAME INPUT ----------------
    if (state == NAME_INPUT)
    {
        DrawText("ENTER NAME:",250,200,40,tamno_zelena);
        DrawText(playerName.c_str(),250,260,40,tamno_zelena);

        int key=GetCharPressed();
        if (key>=32 && key<=125) playerName+=(char)key;

        if (IsKeyPressed(KEY_BACKSPACE)&&!playerName.empty())
            playerName.pop_back();

        if (IsKeyPressed(KEY_ENTER))
        {
            scoreboard.push_back({playerName,igra.score});
            SortScores();
            SaveScores();

            playerName="";
            igra.Reset();
            state=GAME_OVER;
        }
    }

    // ---------------- SCOREBOARD ----------------
 if (state == SCOREBOARD)
{
    selected=0;
    DrawText("SCOREBOARD", CenterText("SCOREBOARD",50),100,50,tamno_zelena);

    float startY = 200;     // Y koordinata prve ocene
    float spacing = 50;      // razmak između redova

    // Crta score tabelu
    for (int i=0;i<scoreboard.size();i++)
    {
        string entry = TextFormat("%d. %s - %d", i+1, scoreboard[i].name.c_str(), scoreboard[i].score);
        DrawText(entry.c_str(), CenterText(entry.c_str(),30), startY + i*spacing, 30, tamno_zelena);
    }

    //Opcije menija (trenutno samo BACK)
    if (IsKeyPressed(KEY_DOWN)) 
    {
        selected++;
    }// jedina opcija
    
    // Crta BACK opciju
    string backText = "> BACK";
    float backY = startY +600;
    DrawText(backText.c_str(), CenterText("BACK",30), backY, 30, tamno_zelena);
    
    if (IsKeyPressed(KEY_ENTER) && selected>=1) 
    {
            state=MENU;
    }
    // ENTER aktivira BACK
    
}
    // ---------------- SETTINGS ----------------
        if (state == SETTINGS)
    {
        const char* settingsOptions[4] = {"MASTER","MUSIC","SFX","BACK"};

        DrawText("SETTINGS", CenterText("SETTINGS",50),100,50,tamno_zelena);

        if (IsKeyPressed(KEY_DOWN)) selected++;
        if (IsKeyPressed(KEY_UP)) selected--;
        selected = (selected+4)%4;

        float startY = 200;   // Y koordinata prve opcije
        float spacing = 70;    // razmak između opcija

        // Crta opcije jedna ispod druge, centrirane
        for(int i=0;i<4;i++)
        {
            float yPos = startY + i*spacing;

            // Izračunaj centriranu X poziciju u odnosu na ime opcije
            int centerX = CenterText(settingsOptions[0],55);

            // Tekst za crtanje (sa strelicom i vrednošću ako je potrebno)
            string displayText = "";
            if(i==selected) displayText += "> ";

            displayText += settingsOptions[i];

            if(i<3) // Dodaj vrednost za MASTER/MUSIC/SFX
            {
                float val = i==0?masterVol:(i==1?musicVol:sfxVol);
                displayText += TextFormat(" %.2f", val);
            }

            DrawText(displayText.c_str(), centerX, yPos, 40, tamno_zelena);
        }
        // LEVO / DESNO za volumene
        if (selected<3)
        {
            if(IsKeyDown(KEY_RIGHT)) { if(selected==0) masterVol+=0.01f; else if(selected==1) musicVol+=0.01f; else sfxVol+=0.01f; PlaySound(moveSound); }
            if(IsKeyDown(KEY_LEFT))  { if(selected==0) masterVol-=0.01f; else if(selected==1) musicVol-=0.01f; else sfxVol-=0.01f; PlaySound(moveSound); }

            masterVol = Clamp(masterVol,0.0f,1.0f);
            musicVol = Clamp(musicVol,0.0f,1.0f);
            sfxVol = Clamp(sfxVol,0.0f,1.0f);
        }

        // ENTER na BACK vraća u MENU
        if(selected==3 && IsKeyPressed(KEY_ENTER)) state=MENU;
    }
    EndDrawing();
}

    CloseAudioDevice();
    CloseWindow();
}