#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;       // Mappa del giorno (Slot 0)
uniform sampler2D nightLightsTex; // Mappa delle luci delle città (Slot 1)
uniform vec2 sunPosGeo;

#define PI 3.14159265358979323846

void main() {
    // 1. Campiona entrambe le texture
    vec4 dayColor = texture(texture0, fragTexCoord);
    vec4 lightColor = texture(nightLightsTex, fragTexCoord);

    // 2. Calcolo coordinate geografiche e angolo del sole
    float lon = (fragTexCoord.x * 2.0 - 1.0) * PI;
    float lat = (1.0 - fragTexCoord.y * 2.0) * (PI / 2.0);
    
    // 3. Coordinate del sole passate dalla CPU
    float sunLon = sunPosGeo.x;
    float sunLat = sunPosGeo.y;
    
    // 4. Calcola l'angolo zenitale del sole (Legge dei coseni sferica)
    // NOTA: Se noti che il sole sorge a Ovest o che l'ombra si muove al contrario, 
    // cambia (lon - sunLon) in (lon + sunLon) o inverti il segno di sunPosGeo.x nel codice C.
    float cosZenith = sin(lat) * sin(sunLat) + cos(lat) * cos(sunLat) * cos(lon - sunLon);


    // 3. Fattore d'ombra (1.0 = Giorno pieno, 0.0 = Notte fonda)
    float shadowFactor = smoothstep(-0.1, 0.1, cosZenith);

    // 4. Calcola la mappa diurna oscurata per la notte (20% di luminosità ambientale)
    vec3 nightMap = dayColor.rgb * 0.20;

    // 5. Calcola l'intensità delle luci delle città
    // (1.0 - shadowFactor) significa che le luci sono a 0 di giorno e a 1 di notte
    // Moltiplichiamo per lightColor.rgb per accendere solo dove ci sono effettivamente le città
    vec3 citiesContent = lightColor.rgb * (1.0 - shadowFactor);

    // 6. Combina la mappa notturna con le luci delle città
    vec3 finalNight = nightMap + citiesContent;

    // 7. Fai il bilanciamento (Interpolazione lineare) tra il giorno e la notte completa
    vec3 finalRGB = mix(finalNight, dayColor.rgb, shadowFactor);

    finalColor = vec4(finalRGB, dayColor.a) * fragColor;
}