#if !defined(UTILS_GLSL)
#define UTILS_GLSL

float luminance(vec3 color) {
    return dot(color, vec3(0.2125, 0.7154, 0.0721));
}

bool moved(mat4 view, mat4 prevView) {
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
            if (abs(view[x][y] - prevView[x][y]) > 0.0001) {
                return true;
            }
        }
    }
    return false;
}

bool isNanVec(vec3 vec) {
    return any(isnan(vec));
}

#endif // UTILS_GLSL