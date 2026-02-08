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

// Output per il Fragment Shader
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out mat3 TBN;

void main()
{
    // 1. Calcolo unico delle UV
    vec2 uv = vertexTexCoord;
    if (useTiling == 1) {
        uv = uv * tiling + offset;
    }

    fragTexCoord = uv;
    fragColor = vertexColor;

    // 2. Trasformazioni di posizione
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    gl_Position = mvp * vec4(vertexPosition, 1.0);

    // 3. Calcolo del Normal Mapping (Spazio Tangente)
    mat3 normalMatrix = mat3(matNormal);

    vec3 N = normalize(normalMatrix * vertexNormal);
    vec3 T = normalize(normalMatrix * vertexTangent.xyz);

    // Riortogonalizzazione di Gram-Schmidt
    T = normalize(T - dot(T, N) * N);

    // Calcolo del bitangente
    vec3 B = cross(N, T) * vertexTangent.w;

    // Matrice TBN
    TBN = mat3(T, B, N);

    // Normale standard per calcoli di luce base
    fragNormal = N;
}
