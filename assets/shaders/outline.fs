#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 resolution;

// Nuovi parametri per il controllo fine
uniform float outlineThickness = 1.0; 
uniform float outlineThreshold = 0.2;
uniform float outlineSoftness = 0.05; // Per bordi meno "pixelosi"

out vec4 finalColor;

// Funzione per calcolare la luminanza
float getLuma(vec2 uv) {
    vec3 col = texture(texture0, uv).rgb;
    return dot(col, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec2 t = outlineThickness / resolution;
    vec2 uv = fragTexCoord;

    // Campionamento 3x3 (Matrice di Sobel)
    // [tl] [tc] [tr]
    // [ml] [cc] [mr]
    // [bl] [bc] [br]
    
    float tl = getLuma(uv + vec2(-t.x,  t.y));
    float tc = getLuma(uv + vec2( 0.0,  t.y));
    float tr = getLuma(uv + vec2( t.x,  t.y));
    
    float ml = getLuma(uv + vec2(-t.x,  0.0));
    float mr = getLuma(uv + vec2( t.x,  0.0));
    
    float bl = getLuma(uv + vec2(-t.x, -t.y));
    float bc = getLuma(uv + vec2( 0.0, -t.y));
    float br = getLuma(uv + vec2( t.x, -t.y));

    // Operatori di Sobel
    float edgeH = tl + (2.0 * ml) + bl - (tr + (2.0 * mr) + br);
    float edgeV = tl + (2.0 * tc) + tr - (bl + (2.0 * bc) + br);
    
    // Magnitudine dell'intensità del bordo
    float edge = sqrt(edgeH * edgeH + edgeV * edgeV);

    // Invece di step(), usiamo smoothstep per ammorbidire l'outline
    float edgeMask = smoothstep(outlineThreshold, outlineThreshold + outlineSoftness, edge);

    vec4 center = texture(texture0, uv);
    vec3 outlineColor = vec3(0.0); // Nero
    
    // Mix finale
    vec3 finalRGB = mix(center.rgb, outlineColor, edgeMask);
    
    finalColor = vec4(finalRGB, center.a);
}
