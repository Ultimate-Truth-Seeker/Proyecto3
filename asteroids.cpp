#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Tamaño de la ventana
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Variables para el control del juego
bool juegoActivo = true;

// Estructuras de los elementos (Nave, Asteroide, Proyectil)
typedef struct {
    int x, y;
    int dx, dy;
    int size;
    int isActive;
} Asteroide;

typedef struct {
    int x, y;
    int vidas;
} Nave;

typedef struct {
    int x, y;
    int dx, dy;
    int isActive;
} Proyectil;

// Variables globales para los elementos del juego
Asteroide* asteroides;
Nave* nave;
Proyectil proyectiles[10];
int numAsteroides = 3;
int numProyectiles = 0;
float anguloNave = 0.0f;  // Ángulo inicial de la nave

// Mutex para sincronización
pthread_mutex_t lock;

// Función para calcular la distancia entre dos objetos
double calcularDistancia(int x1, int y1, int x2, int y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

// Función para mover la nave (se actualizará en cada frame)
void moverNave(sf::RectangleShape& naveShape) {
    float radianes = anguloNave * (3.14159 / 180.0f);  // Convertir ángulo de la nave a radianes

    // Flecha izquierda: Girar en sentido antihorario
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        anguloNave -= 2.0f;  // Ajustar la velocidad de rotación si es necesario
    }

    // Flecha derecha: Girar en sentido horario
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        anguloNave += 2.0f;
    }

    // Flecha arriba: Mover hacia adelante en la dirección del ángulo
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        nave->x += cos(radianes) * 2.0f;  // Cambiar 2.0f para ajustar la velocidad de avance
        nave->y += sin(radianes) * 2.0f;
    }

    // Flecha abajo: Retroceder en la dirección opuesta del ángulo
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        nave->x -= cos(radianes) * 2.0f;  // Cambiar 2.0f para ajustar la velocidad de retroceso
        nave->y -= sin(radianes) * 2.0f;
    }

    // Aplicar la rotación al gráfico de la nave
    naveShape.setRotation(anguloNave);

    // Actualizar la posición gráfica de la nave
    naveShape.setPosition(nave->x, nave->y);
}

void verificarLimitesNave() {
    if (nave->x < 0) nave->x = WINDOW_WIDTH;
    if (nave->x > WINDOW_WIDTH) nave->x = 0;
    if (nave->y < 0) nave->y = WINDOW_HEIGHT;
    if (nave->y > WINDOW_HEIGHT) nave->y = 0;
}

// Colisiones entre proyectiles y asteroides
void manejarColisionProyectilConAsteroide(Proyectil* proyectil, Asteroide* asteroide) {
    if (calcularDistancia(proyectil->x, proyectil->y, asteroide->x, asteroide->y) < (15 * asteroide->size)) {
        proyectil->isActive = 0;  // Destruir el proyectil
        asteroide->isActive = 0;  // Destruir el asteroide original

        // Si el asteroide es grande, se divide en dos pequeños
        if (asteroide->size == 2) {
            for (int i = 0; i < 2; i++) {
                asteroides[numAsteroides].x = asteroide->x;
                asteroides[numAsteroides].y = asteroide->y;
                asteroides[numAsteroides].dx = (rand() % 3) - 1;  // Dirección aleatoria
                asteroides[numAsteroides].dy = (rand() % 3) - 1;
                asteroides[numAsteroides].size = 1;  // Tamaño pequeño
                asteroides[numAsteroides].isActive = 1;
                numAsteroides++;
            }
        }
    }
}

// Colisiones entre la nave y los asteroides
void manejarColisionNaveConAsteroide(Nave* nave, Asteroide* asteroide) {
    if (calcularDistancia(nave->x, nave->y, asteroide->x, asteroide->y) < (15 * asteroide->size)) {
        nave->vidas--;  // Reducir una vida
        if (nave->vidas <= 0) {
            juegoActivo = false;  // Terminar el juego si las vidas se agotan
        }
    }
}

// Función para actualizar los asteroides
void actualizarAsteroides() {
    for (int i = 0; i < numAsteroides; i++) {
        if (asteroides[i].isActive) {
            // Mover los asteroides
            asteroides[i].x += asteroides[i].dx;
            asteroides[i].y += asteroides[i].dy;

            // Rebotar en los bordes
            if (asteroides[i].x < 0 || asteroides[i].x > WINDOW_WIDTH) {
                asteroides[i].dx *= -1;
            }
            if (asteroides[i].y < 0 || asteroides[i].y > WINDOW_HEIGHT) {
                asteroides[i].dy *= -1;
            }

            // Verificar colisiones con la nave
            manejarColisionNaveConAsteroide(nave, &asteroides[i]);
        }
    }
}

void actualizarProyectiles() {
    for (int i = 0; i < 10; i++) {
        if (proyectiles[i].isActive) {
            // Mover el proyectil
            proyectiles[i].x += proyectiles[i].dx;
            proyectiles[i].y += proyectiles[i].dy;

            // Verificar colisiones con los asteroides
            for (int j = 0; j < numAsteroides; j++) {
                if (asteroides[j].isActive) {
                    manejarColisionProyectilConAsteroide(&proyectiles[i], &asteroides[j]);
                }
            }

            // Desactivar el proyectil si sale de la pantalla
            if (proyectiles[i].x < 0 || proyectiles[i].x > WINDOW_WIDTH || proyectiles[i].y < 0 || proyectiles[i].y > WINDOW_HEIGHT) {
                proyectiles[i].isActive = 0;
            }
        }
    }
}

// Función para dibujar los asteroides
void dibujarAsteroides(sf::RenderWindow& window) {
    for (int i = 0; i < numAsteroides; i++) {
        if (asteroides[i].isActive) {
            sf::CircleShape asteroideShape(asteroides[i].size * 15);
            asteroideShape.setPosition(asteroides[i].x, asteroides[i].y);
            asteroideShape.setFillColor(sf::Color::White);
            window.draw(asteroideShape);
        }
    }
}

// Función para dibujar los proyectiles
void dibujarProyectiles(sf::RenderWindow& window) {
    for (int i = 0; i < numProyectiles; i++) {
        if (proyectiles[i].isActive) {
            sf::RectangleShape proyectilShape(sf::Vector2f(5, 10));
            proyectilShape.setPosition(proyectiles[i].x, proyectiles[i].y);
            proyectilShape.setFillColor(sf::Color::Red);
            window.draw(proyectilShape);
        }
    }
}

void dispararProyectil() {
    for (int i = 0; i < 10; i++) {
        // Buscar un proyectil inactivo
        if (!proyectiles[i].isActive) {
            float radianes = anguloNave * (3.14159 / 180.0f);  // Convertir ángulo a radianes

            // Inicializar la posición y dirección del proyectil
            proyectiles[i].x = nave->x + 20;  // Posicionar en la nave
            proyectiles[i].y = nave->y + 20;
            proyectiles[i].dx = cos(radianes) * 5;  // Mover según la dirección de la nave
            proyectiles[i].dy = sin(radianes) * 5;
            proyectiles[i].isActive = 1;
            break;  // Solo disparar un proyectil por pulsación
        }
    }
}
void manejarEntradaDisparo(sf::Event event) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
        dispararProyectil();  // Intentar disparar un proyectil cuando se presiona 'Space'
    }
}


bool quedanAsteroides() {
    for (int i = 0; i < numAsteroides; i++) {
        if (asteroides[i].isActive) {
            return true;  // Si hay algún asteroide activo, el juego sigue
        }
    }
    return false;  // Si no hay asteroides activos, el juego debe terminar
}

// Ciclo principal del juego
int main() {
    // Inicializar SFML y la ventana
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Asteroids - Hand Drawn Style");

    // Crear la nave
    sf::RectangleShape naveShape(sf::Vector2f(40, 40));
    naveShape.setFillColor(sf::Color::Green);

    // Inicializar la nave y los asteroides
    nave = (Nave*)malloc(sizeof(Nave));
    nave->x = 400;
    nave->y = 300;
    nave->vidas = 3;

    asteroides = (Asteroide*)malloc(sizeof(Asteroide) * 10);
    srand(time(NULL));
    for (int i = 0; i < numAsteroides; i++) {
        asteroides[i].x = rand() % WINDOW_WIDTH;
        asteroides[i].y = rand() % WINDOW_HEIGHT;
        asteroides[i].dx =  (((rand() % 3) - 1));
        asteroides[i].dy =  (((rand() % 3) - 1));
        asteroides[i].size = 2;
        asteroides[i].isActive = 1;
    }

    for (int i = 0; i < 10; i++) {
        proyectiles[i].isActive = 0;  // Todos los proyectiles empiezan inactivos
        proyectiles[i].x = 0;
        proyectiles[i].y = 0;
        proyectiles[i].dx = 0;
        proyectiles[i].dy = 0;
    }

    // Bucle principal
    while (window.isOpen() && juegoActivo) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Manejar el evento de disparo
            manejarEntradaDisparo(event);
        }

       // Limpiar la ventana
        window.clear(sf::Color::Black);

        // Mover y rotar la nave
        moverNave(naveShape);
        verificarLimitesNave();

        // Actualizar asteroides y proyectiles
        actualizarAsteroides();
        actualizarProyectiles();

        // Dibujar asteroides, proyectiles y la nave
        dibujarAsteroides(window);
        dibujarProyectiles(window);
        window.draw(naveShape);


        // Verificar si quedan asteroides activos
        if (!quedanAsteroides()) {
            juegoActivo = false;  // Terminar el juego si no hay más asteroides
            // Aquí podrías mostrar un mensaje de "Victoria" o "Juego Terminado"
            sf::Text gameOverText;
            // Configurar y mostrar el texto
        }

        // Mostrar la ventana
        window.display();
        
    }

    // Liberar memoria
    free(asteroides);
    free(nave);

    return 0;
}