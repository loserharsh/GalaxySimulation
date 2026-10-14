#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>

// ============================================================================
// GALAXY SIMULATION CONFIGURATION & CONSTANTS
// ============================================================================
struct GalaxyConfig {
    // Window settings
    unsigned int windowWidth  = 1200;
    unsigned int windowHeight = 900;
    unsigned int frameRate    = 60;

    // Simulation parameters ("The Values")
    size_t numStars           = 8000;      // Total number of simulated stellar bodies
    float  galaxyRadius       = 420.0f;    // Outer disk radius in pixels
    float  coreRadius         = 45.0f;     // Bulge / central core radius
    int    spiralArms         = 2;         // Number of spiral arms (e.g. 2 or 4)
    float  armWindingAngle    = 3.2f;      // How tightly wound the spiral arms are (radians)
    float  armDispersion      = 0.22f;     // Radial & angular dispersion (scatter off the arms)

    // Physics parameters
    float  G                  = 1200.0f;   // Gravitational constant (scaled for screen units)
    float  blackHoleMass      = 15000.0f;  // Mass of central supermassive black hole
    float  darkMatterHaloMass = 35000.0f;  // Distributed dark matter mass (creates flat rotation curves)
    float  softeningLength    = 25.0f;     // Gravitational softening epsilon (prevents infinite slingshots)
    float  timeStep           = 0.15f;     // Simulation delta time per frame
};

// ============================================================================
// STELLAR BODY (STAR)
// ============================================================================
struct Star {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float        mass;
    float        initialDistance;
    sf::Color    color;
};

// ============================================================================
// SIMULATION ENGINE
// ============================================================================
class GalaxySimulation {
public:
    explicit GalaxySimulation(const GalaxyConfig& cfg)
        : config(cfg),
          center(static_cast<float>(cfg.windowWidth) / 2.0f,
                 static_cast<float>(cfg.windowHeight) / 2.0f),
          paused(false),
          starsVertexArray(sf::PrimitiveType::Points)
    {
        stars.reserve(config.numStars);
        starsVertexArray.resize(config.numStars);
        initializeStars();
        printDiagnostics();
    }

    // Prints a breakdown of the physical values and parameters to the console
    void printDiagnostics() const {
        std::cout << "\n======================================================\n";
        std::cout << "          GALAXY SIMULATION ENGINE INITIALIZED        \n";
        std::cout << "======================================================\n";
        std::cout << std::left << std::setw(28) << "  [Window Dimensions]:"
                  << config.windowWidth << " x " << config.windowHeight << " px\n";
        std::cout << std::left << std::setw(28) << "  [Total Stars Count]:"
                  << config.numStars << " stars\n";
        std::cout << std::left << std::setw(28) << "  [Galaxy Disk Radius]:"
                  << config.galaxyRadius << " px\n";
        std::cout << std::left << std::setw(28) << "  [Core Bulge Radius]:"
                  << config.coreRadius << " px\n";
        std::cout << std::left << std::setw(28) << "  [Spiral Arms Count]:"
                  << config.spiralArms << " arms\n";
        std::cout << std::left << std::setw(28) << "  [Central Black Hole Mass]:"
                  << config.blackHoleMass << " M_sol\n";
        std::cout << std::left << std::setw(28) << "  [Dark Matter Halo Mass]:"
                  << config.darkMatterHaloMass << " M_dm\n";
        std::cout << std::left << std::setw(28) << "  [Gravitational Constant G]:"
                  << config.G << "\n";
        std::cout << std::left << std::setw(28) << "  [Softening Length (eps)]:"
                  << config.softeningLength << " px\n";
        std::cout << std::left << std::setw(28) << "  [Time Step (dt)]:"
                  << config.timeStep << "\n";
        std::cout << "------------------------------------------------------\n";
        std::cout << "  CONTROLS:\n";
        std::cout << "   - [Space]      : Pause / Resume simulation\n";
        std::cout << "   - [R]          : Regenerate galaxy with new random seed\n";
        std::cout << "   - [Up/Down]    : Increase / Decrease simulation speed (dt)\n";
        std::cout << "   - [Esc]        : Exit\n";
        std::cout << "======================================================\n\n";
    }

    // Distributes stars into a realistic spiral galaxy with orbital velocities
    void initializeStars() {
        stars.clear();

        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        std::normal_distribution<float>       scatterDist(0.0f, 1.0f);

        for (size_t i = 0; i < config.numStars; ++i) {
            Star star;
            star.mass = 1.0f + dist01(rng) * 2.0f;

            // Non-linear radial distribution: more density toward the galactic core
            float rNorm = std::pow(dist01(rng), 1.6f);
            float r = config.coreRadius * 0.15f + rNorm * config.galaxyRadius;
            star.initialDistance = r;

            // Logarithmic spiral arm placement: theta = armOffset + b * ln(r)
            int armIndex = static_cast<int>(dist01(rng) * config.spiralArms);
            float baseAngle = (2.0f * static_cast<float>(M_PI) * static_cast<float>(armIndex)) / static_cast<float>(config.spiralArms);
            float spiralAngle = baseAngle + config.armWindingAngle * std::log(r / config.coreRadius + 1.0f);

            // Add natural stellar dispersion (scattering off the arms)
            float angularOffset = scatterDist(rng) * config.armDispersion;
            float radialOffset  = scatterDist(rng) * (r * 0.08f);
            float finalAngle    = spiralAngle + angularOffset;
            float finalRadius   = std::max(5.0f, r + radialOffset);

            // 2D position relative to galaxy center
            sf::Vector2f relPos(
                finalRadius * std::cos(finalAngle),
                finalRadius * std::sin(finalAngle)
            );
            star.position = center + relPos;

            // Orbital velocity calculation:
            // Combines central black hole gravity + dark matter halo enclosed mass
            // to produce flat galactic rotation curves: v(r) = sqrt(G * M_enclosed / r)
            float enclosedMass = config.blackHoleMass + config.darkMatterHaloMass * (finalRadius / config.galaxyRadius);
            float vOrbit = std::sqrt((config.G * enclosedMass) / (finalRadius + config.softeningLength));

            // Tangential velocity vector (perpendicular to position vector)
            sf::Vector2f tangent(-relPos.y / finalRadius, relPos.x / finalRadius);
            
            // Add slight random velocity dispersion
            float dispersionVelocity = vOrbit * 0.05f;
            star.velocity = tangent * vOrbit + sf::Vector2f(
                scatterDist(rng) * dispersionVelocity,
                scatterDist(rng) * dispersionVelocity
            );

            // Color coding based on distance:
            // Core: Warm Gold / White
            // Arms: Vibrant Blue / Cyan
            // Outer Rim: Deep Purple / Violet
            float t = finalRadius / config.galaxyRadius;
            if (t < 0.2f) {
                // Hot galactic core
                star.color = sf::Color(255, 240, 200, 240);
            } else if (t < 0.65f) {
                // Young blue stars in spiral arms
                star.color = sf::Color(120, 190, 255, 210);
            } else {
                // Mature stars in the outer fringes
                star.color = sf::Color(190, 140, 255, 170);
            }

            stars.push_back(star);
        }
    }

    // Physics step: Integrates gravity and updates star positions
    void update() {
        if (paused) return;

        float dt = config.timeStep;

        for (size_t i = 0; i < stars.size(); ++i) {
            Star& star = stars[i];

            sf::Vector2f delta = center - star.position;
            float distSq = delta.x * delta.x + delta.y * delta.y;
            float dist   = std::sqrt(distSq);

            // Avoid singularity at center
            if (dist < 1.0f) dist = 1.0f;

            // Enclosed dark matter mass profile: M_halo(r) proportional to r
            float enclosedMass = config.blackHoleMass + config.darkMatterHaloMass * std::min(1.0f, dist / config.galaxyRadius);

            // Gravitational acceleration with softening: a = (G * M) / (r^2 + eps^2)
            float softenedDistSq = distSq + config.softeningLength * config.softeningLength;
            float accMagnitude   = (config.G * enclosedMass) / softenedDistSq;

            sf::Vector2f accDir = delta / dist;
            sf::Vector2f acceleration = accDir * accMagnitude;

            // Symplectic Euler integration
            star.velocity += acceleration * dt;
            star.position += star.velocity * dt;

            // Update vertex array for fast GPU batch rendering
            starsVertexArray[i].position = star.position;
            starsVertexArray[i].color    = star.color;
        }
    }

    void draw(sf::RenderWindow& window) {
        // Draw the central supermassive black hole glow
        sf::CircleShape coreGlow(12.0f);
        coreGlow.setOrigin({12.0f, 12.0f});
        coreGlow.setPosition(center);
        coreGlow.setFillColor(sf::Color(255, 255, 230, 220));
        window.draw(coreGlow);

        // Batch render all stars
        window.draw(starsVertexArray);
    }

    void togglePause() {
        paused = !paused;
        std::cout << "[STATUS]: Simulation " << (paused ? "PAUSED" : "RESUMED") << "\n";
    }

    void adjustSpeed(float delta) {
        config.timeStep = std::max(0.01f, std::min(1.0f, config.timeStep + delta));
        std::cout << "[CONFIG]: Time step (dt) adjusted to: " << config.timeStep << "\n";
    }

    void regenerate() {
        std::cout << "[ACTION]: Regenerating galaxy structure...\n";
        initializeStars();
    }

    const GalaxyConfig& getConfig() const { return config; }

private:
    GalaxyConfig       config;
    sf::Vector2f       center;
    bool               paused;
    std::vector<Star>  stars;
    sf::VertexArray    starsVertexArray;
};

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main() {
    GalaxyConfig config;

    // Create SFML RenderWindow (compatible with SFML 3 and SFML 2)
#if SFML_VERSION_MAJOR >= 3
    sf::RenderWindow window(sf::VideoMode({config.windowWidth, config.windowHeight}),
                            "Galaxy Simulation - N-Body Orbit & Dark Matter Halo");
#else
    sf::RenderWindow window(sf::VideoMode(config.windowWidth, config.windowHeight),
                            "Galaxy Simulation - N-Body Orbit & Dark Matter Halo");
#endif

    window.setFramerateLimit(config.frameRate);

    GalaxySimulation sim(config);

    // Main Game / Simulation Loop
    while (window.isOpen()) {
        // Event Handling
#if SFML_VERSION_MAJOR >= 3
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                } else if (keyPressed->code == sf::Keyboard::Key::Space) {
                    sim.togglePause();
                } else if (keyPressed->code == sf::Keyboard::Key::R) {
                    sim.regenerate();
                } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                    sim.adjustSpeed(+0.03f);
                } else if (keyPressed->code == sf::Keyboard::Key::Down) {
                    sim.adjustSpeed(-0.03f);
                }
            }
        }
#else
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                } else if (event.key.code == sf::Keyboard::Space) {
                    sim.togglePause();
                } else if (event.key.code == sf::Keyboard::R) {
                    sim.regenerate();
                } else if (event.key.code == sf::Keyboard::Up) {
                    sim.adjustSpeed(+0.03f);
                } else if (event.key.code == sf::Keyboard::Down) {
                    sim.adjustSpeed(-0.03f);
                }
            }
        }
#endif

        // Physics update
        sim.update();

        // Render frame
        window.clear(sf::Color(8, 8, 14)); // Deep space dark background
        sim.draw(window);
        window.display();
    }

    return 0;
}
