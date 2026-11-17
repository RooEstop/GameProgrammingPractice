#include <SFML/Graphics.hpp>
#include <iostream>

int main()
{
    sf::RenderWindow window(sf::VideoMode({ 200, 200 }), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                {
                    window.close();
                }

                else if (keyPressed->scancode == sf::Keyboard::Scan::Left)
                {
                    std::cout << "the left arrow was pressed" << std::endl;
                    std::cout << "scancode: " << static_cast<int>(keyPressed->scancode) << std::endl;
                    std::cout << "code: " << static_cast<int>(keyPressed->code) << std::endl;
                    std::cout << "control: " << keyPressed->control << std::endl;
                    std::cout << "alt: " << keyPressed->alt << std::endl;
                    std::cout << "shift: " << keyPressed->shift << std::endl;
                    std::cout << "system: " << keyPressed->system << std::endl;
                    std::cout << "description: " << sf::Keyboard::getDescription(keyPressed->scancode).toAnsiString() << std::endl;
                    std::cout << "localize: " << static_cast<int>(sf::Keyboard::localize(keyPressed->scancode)) << std::endl;
                    std::cout << "delocalize: " << static_cast<int>(sf::Keyboard::delocalize(keyPressed->code)) << std::endl;
                }
            }

            else if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                std::cout << "new width: " << resized->size.x << std::endl;
                std::cout << "new height: " << resized->size.y << std::endl;
            }

            else if (event->is<sf::Event::FocusLost>())
            {
                std::cout << "focus lost" << std::endl;
            }

            else if (event->is<sf::Event::FocusGained>())
            {
                std::cout << "focus gained" << std::endl;
            }

            else if (const auto* textEntered = event->getIf<sf::Event::TextEntered>())
            {
                if (textEntered->unicode < 128)
                    std::cout << "ASCII character typed: " << static_cast<char>(textEntered->unicode) << std::endl;
            }
            


        }

        window.clear();
        window.draw(shape);
        window.display();
    }
}