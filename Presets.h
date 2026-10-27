#pragma once

// az: -180..180 degrees (0 = front, +90 = right). dist: 0..1. height: -1..1. room: 0..1
struct DepthPreset
{
    const char* name;
    float az, dist, height, room;
};

inline const DepthPreset kPresets[] =
{
    { "Piano",  -30.0f, 0.35f,  0.05f, 0.35f },
    { "Guitar",  35.0f, 0.40f,  0.00f, 0.30f },
    { "Vocal",    0.0f, 0.15f,  0.10f, 0.20f },
    { "Bass",     0.0f, 0.30f, -0.20f, 0.10f },
    { "Drums",   10.0f, 0.75f,  0.15f, 0.50f },
    { "Pad",    150.0f, 0.80f,  0.50f, 0.70f }
};
