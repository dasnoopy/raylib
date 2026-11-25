#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

// Conversione da RGB a HSV (Hue, Saturation, Value)
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Conversione da HSV a RGB
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec4 texel = texture(texture0, fragTexCoord);
    vec3 hsv = rgb2hsv(texel.rgb);

    float h = hsv.x; // Tinta (0.0 - 1.0)
    float s = hsv.y; // Saturazione
    float v = hsv.z; // Luminosità

    // 1. GESTIONE DEI VERDI (Boschi e vegetazione: Hue ~0.20 - 0.45)
    // Google Terrain usa un verde salvia molto chiaro e poco saturo
    if (h > 0.18 && h < 0.46 && s > 0.08) {
        h = mix(h, 0.35, 0.45);       // Sposta leggermente verso un verde foglia/salvia
        s *= 0.40;                   // Riduce parecchio la saturazione
        v = mix(v, 0.88, 0.55);      // Schiarisce i boschi scuri di OpenTopoMap
    }
    // 2. GESTIONE ACQUA (Fiumi, laghi, mare: Hue ~0.50 - 0.68)
    // Mantiene l'azzurro riconoscibile ma in stile pastello Google (#aadaff circa)
    else if (h >= 0.48 && h <= 0.68 && s > 0.10) {
        h = mix(h, 0.57, 0.5);       // Azzurro cielo/acqua Google
        s *= 0.72;                   // Mantiene un po' più di colore rispetto alla terra
        v = max(v, 0.85);            // Evita blu troppo scuri
    }
    // 3. GESTIONE TERRE, ROCCE E STRADE (Gialli/Aranci/Marroni: Hue < 0.18 o > 0.90)
    // Trasforma i marroni/arancioni delle montagne in beige/grigio chiaro
    else if (s > 0.08) {
        s *= 0.25;                   // Quasi neutri (beige caldo / tortora)
        v = mix(v, 0.93, 0.50);      // Molto luminosi
    }

    vec3 color = hsv2rgb(vec3(h, s, v));

    // 4. LIFT DELLE OMBRE (Curva stile Google Terrain)
    // Alza i neri (curve di livello e testi troppo scuri) verso un grigio antracite morbido
    // e uniforma lo sfondo chiaro verso un bianco-crema neutro (#f5f5f2)
    vec3 shadowFloor = vec3(0.18, 0.20, 0.22); // Grigio scuro al posto del nero puro
    vec3 highlightTint = vec3(0.97, 0.97, 0.95); // Bianco caldo tipico delle mappe Google

    color = mix(shadowFloor, highlightTint, color);

    finalColor = vec4(color, texel.a) * fragColor;
}
