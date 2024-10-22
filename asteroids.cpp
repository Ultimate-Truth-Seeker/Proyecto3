#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <unistd.h>


// Tamaño de la ventana
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Variables para el control del juego
bool juegoActivo = true;
sf::RenderWindow window(sf::VideoMode(800, 600), "Asteroids Menu");
sf::RectangleShape naveShape(sf::Vector2f(40, 40));
sf::RectangleShape nave2Shape(sf::Vector2f(40, 40));
time_t cooldown = time(nullptr);

// Estructuras de los elementos (Nave, Asteroide, Proyectil)
typedef struct {
    int x, y;
    int dx, dy;
    int size;
    bool isActive;
} Asteroide;

typedef struct {
    float x, y;
    float angulo;
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
Nave* nave2;  // Segunda nave
Proyectil proyectiles[10];
Proyectil proyectiles2[10];
int numAsteroides = 3;
int numProyectiles = 0;
int puntaje = 0;


// Mutex para sincronización

pthread_mutex_t mutexAsteroides;
pthread_mutex_t mutexProyectiles1;
pthread_mutex_t mutexProyectiles2;
pthread_mutex_t mutexNave1;
pthread_mutex_t mutexNave2;
// Función para calcular la distancia entre dos objetos
double calcularDistancia(int x1, int y1, int x2, int y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

// Función para mover la nave (se actualizará en cada frame)
void moverNave(sf::RectangleShape& naveShape) {
    float radianes = nave->angulo * (3.14159 / 180.0f);  // Convertir ángulo a radianes

    // Flecha izquierda: Girar en sentido antihorario
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        nave->angulo -= 2.0f;  // Ajustar la velocidad de rotación si es necesario
    }

    // Flecha derecha: Girar en sentido horario
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        nave->angulo += 2.0f;
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
    naveShape.setRotation(nave->angulo);

    // Actualizar la posición gráfica de la nave
    naveShape.setPosition(nave->x, nave->y);
}

void moverNave2(sf::RectangleShape& naveShape) {
    float radianes = nave2->angulo * (3.14159 / 180.0f);  // Convertir ángulo a radianes

    // Flecha A: Girar en sentido antihorario
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        nave2->angulo -= 2.0f;
    }

    // Flecha D: Girar en sentido horario
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        nave2->angulo += 2.0f;
    }

    // Flecha W: Mover hacia adelante en la dirección del ángulo
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        nave2->x += cos(radianes) * 2.0f;
        nave2->y += sin(radianes) * 2.0f;
    }

    // Flecha S: Retroceder en la dirección opuesta del ángulo
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        nave2->x -= cos(radianes) * 2.0f;
        nave2->y -= sin(radianes) * 2.0f;
    }

    // Aplicar la rotación al gráfico de la nave
    naveShape.setRotation(nave2->angulo);

    // Actualizar la posición gráfica de la nave
    naveShape.setPosition(nave2->x, nave2->y);
}

void verificarLimitesNave() {
    if (nave->x < 0) nave->x = WINDOW_WIDTH;
    if (nave->x > WINDOW_WIDTH) nave->x = 0;
    if (nave->y < 0) nave->y = WINDOW_HEIGHT;
    if (nave->y > WINDOW_HEIGHT) nave->y = 0;
}

void verificarLimitesNave2() {
    if (nave2->x < 0) nave2->x = WINDOW_WIDTH;
    if (nave2->x > WINDOW_WIDTH) nave2->x = 0;
    if (nave2->y < 0) nave2->y = WINDOW_HEIGHT;
    if (nave2->y > WINDOW_HEIGHT) nave2->y = 0;
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
        } else if (asteroide->size == 1) {
            // Incrementar el puntaje al destruir un asteroide pequeño
            puntaje += 10;
        }
    }
}

// Colisiones entre la nave y los asteroides
void manejarColisionNaveConAsteroide(Nave* nave, Asteroide* asteroide) {
    if (calcularDistancia(nave->x, nave->y, asteroide->x, asteroide->y) < (15 * asteroide->size)) {
        if (difftime(time(nullptr), cooldown) > 2) {
            nave->vidas--;  // Reducir una vida
            cooldown = time(nullptr);
        } 
        
    }
}

// Función para actualizar los asteroides
void actualizarAsteroides() {
    pthread_mutex_lock(&mutexAsteroides);
    pthread_mutex_lock(&mutexNave1);
    pthread_mutex_lock(&mutexNave2);
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
    pthread_mutex_unlock(&mutexNave2);
    pthread_mutex_unlock(&mutexNave1);
    pthread_mutex_unlock(&mutexAsteroides);
}


void* hiloNave1Func(void* arg) {
    while (juegoActivo) {
        pthread_mutex_lock(&mutexNave1);
        moverNave(naveShape);
        verificarLimitesNave();
        pthread_mutex_unlock(&mutexNave1);
        usleep(16000); // Aproximadamente 60 FPS
    }
    return NULL;
}
void* hiloAsteroidesFunc(void* arg) {
    while (juegoActivo) {
        pthread_mutex_lock(&mutexAsteroides);
        actualizarAsteroides();
        pthread_mutex_unlock(&mutexAsteroides);
        usleep(16000);
    }
    return NULL;
}


void actualizarProyectiles() {
    pthread_mutex_lock(&mutexAsteroides);
    pthread_mutex_lock(&mutexProyectiles1);
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
    pthread_mutex_unlock(&mutexProyectiles1);
    pthread_mutex_unlock(&mutexAsteroides);
}

void actualizarProyectilesNave2() {
    pthread_mutex_lock(&mutexAsteroides);
    pthread_mutex_lock(&mutexProyectiles2);
    for (int i = 0; i < 10; i++) {
        if (proyectiles2[i].isActive) {
            // Mover el proyectil
            proyectiles2[i].x += proyectiles2[i].dx;
            proyectiles2[i].y += proyectiles2[i].dy;

            // Verificar colisiones con los asteroides
            for (int j = 0; j < numAsteroides; j++) {
                if (asteroides[j].isActive) {
                    manejarColisionProyectilConAsteroide(&proyectiles2[i], &asteroides[j]);
                }
            }

            // Desactivar el proyectil si sale de la pantalla
            if (proyectiles2[i].x < 0 || proyectiles2[i].x > WINDOW_WIDTH || proyectiles2[i].y < 0 || proyectiles2[i].y > WINDOW_HEIGHT) {
                proyectiles2[i].isActive = 0;
            }
        }
    }
    pthread_mutex_unlock(&mutexProyectiles2);
    pthread_mutex_unlock(&mutexAsteroides);
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

void* hiloProyectiles1Func(void* arg) {
    while (juegoActivo) {
        pthread_mutex_lock(&mutexProyectiles1);
        actualizarProyectiles();
        pthread_mutex_unlock(&mutexProyectiles1);
        usleep(16000);
    }
    return NULL;
}
void* hiloProyectiles2Func(void* arg) {
    while (juegoActivo) {
        pthread_mutex_lock(&mutexProyectiles2);
        actualizarProyectilesNave2();
        pthread_mutex_unlock(&mutexProyectiles2);
        usleep(16000);
    }
    return NULL;
}

// Función para dibujar los proyectiles
void dibujarProyectiles(sf::RenderWindow& window) {
    for (int i = 0; i < 10; i++) {
        if (proyectiles[i].isActive) {
            sf::RectangleShape proyectilShape(sf::Vector2f(5, 10));
            proyectilShape.setPosition(proyectiles[i].x, proyectiles[i].y);
            proyectilShape.setFillColor(sf::Color::Red);
            window.draw(proyectilShape);
        }
    }
}

void dibujarProyectilesNave2(sf::RenderWindow& window) {
    for (int i = 0; i < 10; i++) {
        if (proyectiles2[i].isActive) {
            sf::RectangleShape proyectilShape(sf::Vector2f(5, 10));
            proyectilShape.setPosition(proyectiles2[i].x, proyectiles2[i].y);
            proyectilShape.setFillColor(sf::Color::Red);
            window.draw(proyectilShape);
        }
    }
}

void dispararProyectil() {
    for (int i = 0; i < 10; i++) {
        // Buscar un proyectil inactivo
        if (!proyectiles[i].isActive) {
            float radianes = nave->angulo * (3.14159 / 180.0f);  // Convertir ángulo a radianes

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
void dispararProyectilNave2() {
    for (int i = 0; i < 10; i++) {
        if (!proyectiles2[i].isActive) {
            float radianes = nave2->angulo * (3.14159 / 180.0f);  // Convertir ángulo a radianes

            // Inicializar la posición y dirección del proyectil
            proyectiles2[i].x = nave2->x + 20;
            proyectiles2[i].y = nave2->y + 20;
            proyectiles2[i].dx = cos(radianes) * 5.0f;
            proyectiles2[i].dy = sin(radianes) * 5.0f;
            proyectiles2[i].isActive = 1;
            break;
        }
    }
}
void manejarEntradaDisparo(sf::Event event) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
        dispararProyectil();  // Intentar disparar un proyectil cuando se presiona 'Space'
    }
}

void manejarEntradaDisparoNave2(sf::Event event) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
        dispararProyectilNave2();
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


sf::Font font;  // Fuente para el texto del menú
sf::Text menuOptions[3];  // Opciones del menú
int selectedOption = 0;  // Opción seleccionada



void mostrarInstrucciones(sf::RenderWindow& window) {
    sf::Text instrucciones;
    instrucciones.setFont(font);
    instrucciones.setString("Instrucciones del Juego:\n- Usa las flechas para moverte.\n- Presiona Space para disparar.");
    instrucciones.setPosition(100, 200);
    instrucciones.setFillColor(sf::Color::White);

    // Mostrar la pantalla de instrucciones
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                return;  // Volver al menú principal cuando se presione Enter
            }
        }

        window.clear();
        window.draw(instrucciones);
        window.display();
    }
}


void ejecutarJuegoUnJugador(sf::RenderWindow& window) {
    // Configura la nave, los asteroides y otros elementos como en tu `main` anterior
    numAsteroides = 3;
    juegoActivo = true;  // Restablecer 'juegoActivo' al iniciar el juego
    nave = new Nave();   // Asignar memoria a 'nave'
    asteroides = new Asteroide[20];  // Por ejemplo, un máximo de 20 asteroides


    nave->x = 400;
    nave->y = 300;
    nave->angulo = 0;
    nave->vidas = 3;

    sf::Text textoVidas;
    sf::Text textoPuntaje;
    textoPuntaje.setFont(font);  // Asegúrate de que la fuente está cargada
    textoPuntaje.setCharacterSize(24);
    textoPuntaje.setFillColor(sf::Color::White);
    textoPuntaje.setPosition(10, 40);  // Posición en la pantalla

    textoVidas.setFont(font);  // Asegúrate de que la fuente está cargada
    textoVidas.setCharacterSize(24);
    textoVidas.setFillColor(sf::Color::White);
    textoVidas.setPosition(10, 70);  // Posición en la pantalla

    for (int i = 0; i < numAsteroides; i++) {
        asteroides[i].x = rand() % WINDOW_WIDTH;
        asteroides[i].y = rand() % WINDOW_HEIGHT;
        asteroides[i].dx =  (((rand() % 3) - 1));
        asteroides[i].dy =  (((rand() % 3) - 1));
        asteroides[i].size = 2;
        asteroides[i].isActive = 1;
    }

    

    while (window.isOpen() && juegoActivo) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            manejarEntradaDisparo(event);  // Detectar disparo de un jugador
        }
        textoPuntaje.setString("Puntaje: " + std::to_string(puntaje));
        textoVidas.setString("Vidas: " + std::to_string(nave->vidas));

        pthread_mutex_lock(&mutexAsteroides);
        pthread_mutex_lock(&mutexProyectiles1);
        pthread_mutex_lock(&mutexNave1);
        // Limpiar la ventana
        window.clear(sf::Color::Black);
        
        window.draw(textoPuntaje);
        window.draw(textoVidas);
        // Mover la nave y actualizar el juego
        moverNave(naveShape);
        verificarLimitesNave();

        // Actualizar asteroides y proyectiles
        actualizarAsteroides();
        actualizarProyectiles();

        // Dibujar asteroides, proyectiles y la nave
        dibujarAsteroides(window);
        dibujarProyectiles(window);
        window.draw(naveShape);

        pthread_mutex_unlock(&mutexNave1);
        pthread_mutex_unlock(&mutexProyectiles1);
        pthread_mutex_unlock(&mutexAsteroides);

        // Mostrar la ventana
        window.display();
        if (!quedanAsteroides()) {
            // Mostrar mensaje de victoria (opcional)
            

            window.clear();
            window.display();
            juegoActivo = false;  // Terminar el juego
        }
        if (nave->vidas <= 0) {
            // Mostrar mensaje de derrota (opcional)
            

            window.clear();
            window.display();
            juegoActivo = false;  // Terminar el juego
        }
    }
    delete nave;

    delete[] asteroides;
}

void ejecutarJuegoDosJugadores(sf::RenderWindow& window) {
    // Configurar naves, asteroides y otros elementos
    numAsteroides = 5;
    juegoActivo = true;
    nave = new Nave();   // Asignar memoria a 'nave'
    nave2 = new Nave();  // Asignar memoria a 'nave2'
    asteroides = new Asteroide[20];  // Por ejemplo, un máximo de 20 asteroides

    sf::Text textoVidas;
    sf::Text textoVidas2;
    sf::Text textoPuntaje;
    textoPuntaje.setFont(font);  // Asegúrate de que la fuente está cargada
    textoPuntaje.setCharacterSize(24);
    textoPuntaje.setFillColor(sf::Color::White);
    textoPuntaje.setPosition(10, 40);  // Posición en la pantalla

    textoVidas.setFont(font);  // Asegúrate de que la fuente está cargada
    textoVidas.setCharacterSize(24);
    textoVidas.setFillColor(sf::Color::White);
    textoVidas.setPosition(10, 70);  // Posición en la pantalla
    textoVidas2.setFont(font);  // Asegúrate de que la fuente está cargada
    textoVidas2.setCharacterSize(24);
    textoVidas2.setFillColor(sf::Color::White);
    textoVidas2.setPosition(10, 100);  // Posición en la pantalla

    nave->x = 200;
    nave->y = 300;
    nave->angulo = 0;
    nave->vidas = 3;
    nave2->x = 500;
    nave2->y = 300;
    nave2->angulo = 0;
    nave2->vidas = 3;
    for (int i = 0; i < numAsteroides; i++) {
        asteroides[i].x = rand() % WINDOW_WIDTH;
        asteroides[i].y = rand() % WINDOW_HEIGHT;
        asteroides[i].dx =  (((rand() % 3) - 1));
        asteroides[i].dy =  (((rand() % 3) - 1));
        asteroides[i].size = 2;
        asteroides[i].isActive = 1;
    }

    

    while (window.isOpen() && juegoActivo) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            manejarEntradaDisparo(event);       // Disparo para el jugador 1
            manejarEntradaDisparoNave2(event);  // Disparo para el jugador 2
        }

        textoPuntaje.setString("Puntaje: " + std::to_string(puntaje));
        textoVidas.setString("Vidas 1: " + std::to_string(nave->vidas));
        textoVidas2.setString("Vidas 2: " + std::to_string(nave2->vidas));
        
        pthread_mutex_lock(&mutexAsteroides);
        pthread_mutex_lock(&mutexProyectiles1);
        pthread_mutex_lock(&mutexNave1);
        pthread_mutex_lock(&mutexNave2);

        // Limpiar la ventana
        window.clear(sf::Color::Black);
        window.draw(textoPuntaje);
        window.draw(textoVidas);
        window.draw(textoVidas2);

        // Mover y rotar la nave 1
        moverNave(naveShape);
        verificarLimitesNave();

        // Mover y rotar la nave 2
        moverNave2(nave2Shape);
        verificarLimitesNave2();  // Implementa esta función similar a la de la primera nave

        // Actualizar asteroides y proyectiles
        actualizarAsteroides();
        actualizarProyectiles();
        actualizarProyectilesNave2();  // Implementa esta función similar a la de la primera nave

        // Dibujar asteroides, proyectiles y las naves
        dibujarAsteroides(window);
        dibujarProyectiles(window);
        dibujarProyectilesNave2(window);
        window.draw(naveShape);
        window.draw(nave2Shape);

        pthread_mutex_unlock(&mutexNave1);
        pthread_mutex_unlock(&mutexNave2);
        pthread_mutex_unlock(&mutexProyectiles1);
        pthread_mutex_unlock(&mutexAsteroides);

        // Mostrar la ventana
        window.display();
        if (!quedanAsteroides()) {
            // Mostrar mensaje de victoria (opcional)
            

            window.clear();
            window.display();
            juegoActivo = false;  // Terminar el juego
        }
        if (nave->vidas <= 0 || nave2->vidas <= 0) {
            // Mostrar mensaje de derrota (opcional)
            

            window.clear();
            window.display();
            juegoActivo = false;  // Terminar el juego
        }
    }
    delete nave;
    delete nave2;
    delete[] asteroides;
}
void seleccionarOpcionMenu(sf::RenderWindow& window) {
    if (selectedOption == 0) {
        mostrarInstrucciones(window);
    } else if (selectedOption == 1) {
        ejecutarJuegoUnJugador(window);
    } else if (selectedOption == 2) {
        ejecutarJuegoDosJugadores(window);
    }
}
void manejarEntradaMenu(sf::RenderWindow& window, sf::Event event) {
    // Navegación con teclas
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Up) {
            selectedOption = (selectedOption - 1 + 3) % 3;  // Mover hacia arriba en el menú
        }
        if (event.key.code == sf::Keyboard::Down) {
            selectedOption = (selectedOption + 1) % 3;  // Mover hacia abajo en el menú
        }

        // Resaltar la opción seleccionada
        for (int i = 0; i < 3; i++) {
            menuOptions[i].setFillColor(sf::Color::White);  // Restaurar color blanco para todas
        }
        menuOptions[selectedOption].setFillColor(sf::Color::Red);  // Resaltar la seleccionada
    }

    // Manejar clic del ratón
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

        // Verificar si el clic está dentro de los límites de alguna opción del menú
        for (int i = 0; i < 3; i++) {
            sf::FloatRect bounds = menuOptions[i].getGlobalBounds();  // Obtener los límites de la opción
            if (bounds.contains(static_cast<float>(mousePosition.x), static_cast<float>(mousePosition.y))) {
                selectedOption = i;  // Establecer la opción seleccionada según el clic
                seleccionarOpcionMenu(window);  // Llamar a la función para procesar la opción seleccionada
            }
        }
    }
}

void mostrarMenu(sf::RenderWindow& window) {
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Manejar la entrada del teclado y ratón
            manejarEntradaMenu(window, event);

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                seleccionarOpcionMenu(window);  // Procesar la opción seleccionada con Enter
            }
        }

        window.clear();
        for (int i = 0; i < 3; i++) {
            window.draw(menuOptions[i]);
        }
        window.display();
    }
}

// Ciclo principal del juego
int main() {
    // Inicializar SFML y la ventana
    if (!font.loadFromFile("Arial.ttf")) {
    // Manejar error
    }

    // Configurar las opciones del menú
    menuOptions[0].setFont(font);
    menuOptions[0].setString("Ver Instrucciones");
    menuOptions[0].setPosition(300, 200);

    menuOptions[1].setFont(font);
    menuOptions[1].setString("Modo Un Jugador");
    menuOptions[1].setPosition(300, 300);

    menuOptions[2].setFont(font);
    menuOptions[2].setString("Modo Dos Jugadores");
    menuOptions[2].setPosition(300, 400);

    // Configurar el color de la opción seleccionada
    for (int i = 0; i < 3; i++) {
        menuOptions[i].setFillColor(sf::Color::White);  // Color por defecto
    }
    
    // Definir las posiciones iniciales de las naves
    naveShape.setPosition(400, 300);  // Centrada en la ventana
    naveShape.setSize(sf::Vector2f(40, 40));  // Asegúrate de que la nave sea visible

    nave2Shape.setPosition(200, 300);  // Posicionar la segunda nave
    nave2Shape.setSize(sf::Vector2f(40, 40));  // Asegúrate de que la segunda nave sea visible

    // Llenar las naves con un color visible
    naveShape.setFillColor(sf::Color::Green);
    nave2Shape.setFillColor(sf::Color::Green);

    menuOptions[selectedOption].setFillColor(sf::Color::Red);  // Opción seleccionada en rojo
    // Mostrar el menú principal
    window.setFramerateLimit(60);  // Limitar a 60 FPS

    mostrarMenu(window);

    return 0;
}