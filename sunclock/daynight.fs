#version 330

// Input dal vertex shader di raylib
in vec2 fragTexCoord;
in vec4 fragColor;

// Output del pixel
out vec4 finalColor;

// Uniforms passate dal codice C
uniform sampler2D texture0;      // La tua cartina del mondo
uniform vec2 sunPosGeo;          // x = Longitudine Sole, y = Latitudine Sole (in radianti)

#define PI 3.14159265358979323846

void main() {
    // 1. Campiona il colore originale della cartina
    vec4 texelColor = texture(texture0, fragTexCoord);

    // 2. Converti le coordinate UV del pixel (0.0 a 1.0) in coordinate Geografiche (radianti)
    // UV (0,0) è in alto a sinistra. Mappa sferica piatta standard:
    float lon = (fragTexCoord.x * 2.0 - 1.0) * PI;       // Da -PI a +PI
    float lat = (1.0 - fragTexCoord.y * 2.0) * (PI / 2.0); // Da +PI/2 a -PI/2

    // 3. Calcola l'angolo zenitale del sole per questo punto (Legge dei coseni sferica)
    float sunLat = sunPosGeo.y;
    float sunLon = sunPosGeo.x;
    
    float cosZenith = sin(lat) * sin(sunLat) + cos(lat) * cos(sunLat) * cos(lon - sunLon);

    // 4. Determina l'illuminazione
    // cosZenith > 0 significa Giorno, < 0 significa Notte.
    // Usiamo smoothstep per creare una sfumatura morbida (il crepuscolo)
    float shadowFactor = smoothstep(-0.1, 0.1, cosZenith);

    // Applica un moltiplicatore per la zona d'ombra (es: 0.25 rende la notte scura ma visibile)
    float ambientNight = 0.25;
    float illumination = ambientNight + (1.0 - ambientNight) * shadowFactor;

    finalColor = vec4(texelColor.rgb * illumination, texelColor.a) * fragColor;
}
