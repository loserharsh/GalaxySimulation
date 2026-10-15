#include <SFML/Graphics.hpp>

int main() {
    // Step 0: Minimal starter window
    sf::RenderWindow window(sf::VideoMode({1000, 800}), "Galaxy Simulation");
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        window.clear(sf::Color::Black);
        window.display();
    }

    return 0;
}
