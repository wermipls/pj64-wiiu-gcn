// simple dz (no scale)
int deadzone(int val, int dz)
{
    if (val > 0) {
        val -= dz;
        if (val < 0) return 0;
    }
    if (val < 0) {
        val += dz;
        if (val > 0) return 0;
    }

    return val;
}
