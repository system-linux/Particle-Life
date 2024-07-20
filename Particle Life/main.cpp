#include <iostream>
#include <format>
#include <random>
#include <numeric>
#include <SFML/Graphics.hpp>
typedef short int sint;
const int canvaX = 1920, canvaY = 1080;
constexpr sint particleSize = 5;
constexpr int particleNum = 10000;

using namespace std;

struct Particle {
    float x, y, vx = 0, vy = 0;
    double agressiveness = 0.0;
    int no;
    bool isDead = false;
    sf::Color color;
    Particle(float x, float y, sf::Color color, int no) : x(x), y(y), color(color), no(no) {}

    bool operator==(const Particle& other) const = default;
};
vector<Particle> particles;

static vector<Particle> create(int number, sf::Color color) {
    vector<Particle> group;
    for (int i = 1; i <= number; ++i) {
        int x = static_cast<float>(rand()) / RAND_MAX * (canvaX - 10) + 10;
        int y = static_cast<float>(rand()) / RAND_MAX * (canvaY - 10) + 10;
        group.emplace_back(Particle(x, y, color, i));
    }
    return group;
}
vector<Particle> yellow = create(particleNum, sf::Color::Yellow);
vector<Particle> red = create(particleNum, sf::Color::Red);
vector<Particle> green = create(particleNum, sf::Color::Green);
sint getDistance(Particle& a, Particle& b) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return static_cast<int>(sqrt(dx * dx + dy * dy));
}

sf::Color colors[] = {
    sf::Color::Red,
    sf::Color::Yellow,
    sf::Color::Green
};

sf::Color getRandomColor() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(0, sizeof(colors) / sizeof(colors[0]) - 1);
    return colors[distr(gen)];
}

static double calculate_aggressiveness(float opponent_aggressiveness, int distance, int num_nearby_friends, int num_nearby_enemies, float average_friend_aggressiveness) {
    float A1 = 1 / (1 + distance);
    float A2 = (num_nearby_friends > num_nearby_enemies) ? 0.3f : -0.2f;
    float aggressiveness = std::clamp(A1 + A2 + 0.4f * average_friend_aggressiveness + 0.4f * opponent_aggressiveness, 0.0f, 1.0f);
    return aggressiveness;
}

vector<Particle> NearbyParticles(Particle& mainParticle, sf::Color color, int distanceLimit = 20) {
    vector<Particle> nearbyParticles;
    for (Particle& particle : particles) {
        if (particle.color == color && getDistance(mainParticle, particle) <= distanceLimit) {
            nearbyParticles.push_back(particle);
        }
    }
    sort(nearbyParticles.begin(), nearbyParticles.end(), [&](Particle& a, Particle& b) {
        return getDistance(mainParticle, a) < getDistance(mainParticle, b);
        });
    return nearbyParticles;
}

static void movementRule(vector<Particle>& affected, vector<Particle>& influencer, float g) {
    for (auto& a : affected) {
        float fx = 0, fy = 0;
        for (auto& b : influencer) {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            float d = sqrt(dx * dx + dy * dy);
            if (d > 0 && d < 80) {
                float F = g / d;
                fx += F * dx;
                fy += F * dy;
            }
        }
        a.vx = (a.vx + fx) * 0.5f;
        a.vy = (a.vy + fy) * 0.5f;
        a.x += a.vx;
        a.y += a.vy;

        if (a.x <= 0 || a.x >= canvaX - particleSize) a.x = -a.vx / 2;
        if (a.y <= 0 || a.y >= canvaY - particleSize) a.y = -a.vy / 2;
    }
}

void interactionRule() {
    sf::Color randColor = getRandomColor();
    auto killParticle = [&](vector<Particle>& group, Particle& target) {
        auto it = find_if(group.begin(), group.end(), [&target](const Particle& p) {
            return p.no == target.no;
            });
        if (it != group.end()) {
            it->isDead = true;
            //cout << target.no << " deleted.\n\n";
        }
        else {
            cout << "Can't find particle with number " << target.no << ".\n\n";
        }
        };
    for (Particle& particle : particles) {
        while (particle.color != randColor) randColor = getRandomColor();
        vector<Particle> nearbyEnemies = NearbyParticles(particle, randColor);
        vector<Particle> friends = NearbyParticles(particle, particle.color, 30);
        double averageFriendAggressiveness = accumulate(friends.begin(), friends.end(), 0.0f, [&](float sum, const Particle& friendP) {
            return sum + friendP.agressiveness;
            }) / friends.size();
        sint distance = getDistance(particle, nearbyEnemies[0]);
        double agresiveness = calculate_aggressiveness(nearbyEnemies[0].agressiveness, distance, friends.size(),
            nearbyEnemies.size(), averageFriendAggressiveness);
        if (distance == 0 && friends.size() > 4 && nearbyEnemies.size() > 3) particle.agressiveness /= 2; // Data anomaly check
        else particle.agressiveness = agresiveness;
    }
    for (Particle& particle : particles) {
        while (particle.color != randColor) randColor = getRandomColor();
        vector<Particle> nearbyEnemies = NearbyParticles(particle, randColor);
        if (!nearbyEnemies.empty() && particle.agressiveness > nearbyEnemies[0].agressiveness) {
            if (particle.color == sf::Color::Yellow) killParticle(yellow, nearbyEnemies[0]);
            else if (particle.color == sf::Color::Red) killParticle(red, nearbyEnemies[0]);
            else if (particle.color == sf::Color::Green) killParticle(green, nearbyEnemies[0]);
        }
    }
    yellow.erase(remove_if(yellow.begin(), yellow.end(), [](const Particle& p) { return p.isDead; }), yellow.end());
    red.erase(remove_if(red.begin(), red.end(), [](const Particle& p) { return p.isDead; }), red.end());
    green.erase(remove_if(green.begin(), green.end(), [](const Particle& p) { return p.isDead; }), green.end());
}

int main() {
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(canvaX, canvaY), "Life Simulation");
    cout << "Started!\n";

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
        }
        particles.clear();
        particles.insert(particles.end(), yellow.begin(), yellow.end());
        particles.insert(particles.end(), red.begin(), red.end());
        particles.insert(particles.end(), green.begin(), green.end());

        movementRule(green, green, -0.22);
        movementRule(green, red, -0.17f);
        movementRule(green, yellow, 0.34f);
        movementRule(red, red, -0.1f);
        movementRule(red, green, -0.34f);
        movementRule(yellow, yellow, 0.15f);
        movementRule(yellow, green, -0.2f);
        interactionRule();
        cout << format("Yellow -> {}\nRed -> {}\nGreen -> {}\n", yellow.size(), red.size(), green.size());
        window.clear();
        for (const auto& p : particles) {
            sf::RectangleShape rectangle(sf::Vector2f(particleSize, particleSize));
            rectangle.setPosition(p.x, p.y);
            rectangle.setFillColor(p.color);
            window.draw(rectangle);
        }
        window.display();
    }
}