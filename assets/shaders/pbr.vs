#version 330

// Attributi di ingresso (dai buffer della mesh)
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexTangent;
in vec4 vertexColor;

// Uniforms
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal; // Usiamo questa anziché calcolare l'inversa in ogni frame

uniform vec2 tiling;
uniform vec2 offset;
uniform int useTiling;

// Shadow mapping
uniform mat4 lightSpaceMatrix;

// Output per il Fragment Shader
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out mat3 TBN;
out vec4 fragPosLightSpace;

void main()
{
    // 1. Calcolo unico delle UV
    // Partiamo dall'input e applichiamo tiling/offset se richiesto
    vec2 uv = vertexTexCoord;
    if (useTiling == 1) {
        uv = uv * tiling + offset;
    }

    // Passiamo il risultato al fragment shader
    fragTexCoord = uv;
    fragColor = vertexColor;

    // 2. Trasformazioni di posizione
    // Calcoliamo la posizione nel mondo (world space)
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));

    // Calcoliamo la posizione finale a schermo
    gl_Position = mvp * vec4(vertexPosition, 1.0);

    // 3. Calcolo del Normal Mapping (Spazio Tangente)
    // Usiamo mat3(matNormal) per trasformare i vettori normali correttamente
    mat3 normalMatrix = mat3(matNormal);

    vec3 N = normalize(normalMatrix * vertexNormal);
    vec3 T = normalize(normalMatrix * vertexTangent.xyz);

    // Riortogonalizzazione di Gram-Schmidt per correggere eventuali errori di precisione
    T = normalize(T - dot(T, N) * N);

    // Calcolo del bitangente (Binormal)
    // Il prodotto vettoriale tra N e T ci dà il terzo asse
    vec3 B = cross(N, T) * vertexTangent.w;

    // Costruiamo la matrice TBN (Tangent, Bitangent, Normal)
    // La passiamo al fragment shader per trasformare le normali della mappa
    TBN = mat3(T, B, N);

    // Passiamo anche la normale standard per calcoli di luce base
    fragNormal = N;

    // 4. Shadow mapping: transform world position to light space
    fragPosLightSpace = lightSpaceMatrix * vec4(fragPosition, 1.0);
}
