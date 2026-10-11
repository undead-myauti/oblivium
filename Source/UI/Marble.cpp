#include "Marble.h"
#include "ObliviumLookAndFeel.h"

#include <thread>

namespace oblivium::ui
{
namespace
{
    // Texture size in pixels. The enclosure is 424 x 644 base pixels; 2x keeps the
    // veins crisp on HiDPI screens and when the window is enlarged.
    constexpr int textureWidth = 848;
    constexpr int textureHeight = 1288;
    constexpr float unitsPerPixel = 0.5f;
    constexpr juce::uint32 marbleSeed = 0x0b11u;

    float hash (int x, int y, juce::uint32 seed) noexcept
    {
        auto h = (juce::uint32) x * 374761393u + (juce::uint32) y * 668265263u + seed * 2246822519u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (float) (h & 0xffffffu) / (float) 0xffffffu;
    }

    float valueNoise (float x, float y, juce::uint32 seed) noexcept
    {
        const auto xi = (int) std::floor (x), yi = (int) std::floor (y);
        const auto xf = x - (float) xi, yf = y - (float) yi;
        const auto u = xf * xf * (3.0f - 2.0f * xf);
        const auto v = yf * yf * (3.0f - 2.0f * yf);

        const auto top    = juce::jmap (u, hash (xi, yi, seed),     hash (xi + 1, yi, seed));
        const auto bottom = juce::jmap (u, hash (xi, yi + 1, seed), hash (xi + 1, yi + 1, seed));
        return juce::jmap (v, top, bottom);
    }

    /** Fractal noise, roughly 0..1. */
    float fbm (float x, float y, juce::uint32 seed, int octaves) noexcept
    {
        float sum = 0.0f, amplitude = 0.5f, total = 0.0f;

        for (int i = 0; i < octaves; ++i)
        {
            sum += amplitude * valueNoise (x, y, seed + (juce::uint32) i * 101u);
            total += amplitude;
            x = x * 2.03f + 17.0f;
            y = y * 2.03f + 31.0f;
            amplitude *= 0.5f;
        }

        return sum / total;
    }

    /** 1 on the zero crossings of a sine, falling off sharply: a thin vein. */
    float ridge (float phase, int sharpness) noexcept
    {
        auto r = 1.0f - std::abs (std::sin (phase));
        for (int i = 0; i < sharpness; ++i)
            r *= r;
        return r;
    }

    void renderMarbleRows (const juce::Image::BitmapData& pixels, int firstRow, int endRow)
    {
        for (int y = firstRow; y < endRow; ++y)
        {
            for (int x = 0; x < textureWidth; ++x)
            {
                const auto u = (float) x * unitsPerPixel;
                const auto v = (float) y * unitsPerPixel;

                // Domain-warped noise bends the veins the way they flow in real stone:
                // long diagonal sweeps with a finer secondary network.
                const auto warp = fbm (u * 0.004f, v * 0.004f, marbleSeed + 11u, 4);
                const auto flow = fbm (u * 0.007f + 2.4f * warp, v * 0.007f + 1.3f * warp, marbleSeed + 23u, 5);

                const auto mainVein = ridge ((u * 0.55f + v * 0.85f) * 0.016f + flow * 6.0f, 4);
                const auto fineVein = ridge ((u * 0.8f - v * 0.45f) * 0.024f + flow * 7.5f + warp * 3.0f, 5);
                const auto clouds = fbm (u * 0.018f, v * 0.018f, marbleSeed + 37u, 4);

                const auto luminance = 0.03f
                                     + clouds * 0.04f
                                     + mainVein * 0.16f * (0.35f + warp)
                                     + fineVein * 0.06f * clouds;

                const auto level = [luminance] (float tint)
                {
                    return (juce::uint8) juce::jlimit (0, 255, juce::roundToInt (luminance * tint * 255.0f));
                };

                // Slightly cool greys, like Nero Marquina.
                reinterpret_cast<juce::PixelARGB*> (pixels.getPixelPointer (x, y))
                    ->setARGB (255, level (0.97f), level (0.98f), level (1.04f));
            }
        }
    }

    juce::Image createMarble()
    {
        juce::Image image (juce::Image::ARGB, textureWidth, textureHeight, false, juce::SoftwareImageType());
        const juce::Image::BitmapData pixels (image, juce::Image::BitmapData::writeOnly);

        // Rows are independent, so split them across the cores; the editor opens noticeably faster.
        const auto numThreads = (int) juce::jlimit (1u, 8u, std::thread::hardware_concurrency());
        const auto rowsPerThread = (textureHeight + numThreads - 1) / numThreads;
        std::vector<std::thread> workers;

        for (int i = 1; i < numThreads; ++i)
            workers.emplace_back ([&pixels, i, rowsPerThread]
            {
                renderMarbleRows (pixels, i * rowsPerThread, juce::jmin (textureHeight, (i + 1) * rowsPerThread));
            });

        renderMarbleRows (pixels, 0, juce::jmin (textureHeight, rowsPerThread));

        for (auto& worker : workers)
            worker.join();

        return image;
    }

    struct Crack
    {
        juce::Point<float> start;
        float heading;   // radians, 0 = right, y grows downwards
        float length;
        float width;
        int branches;    // how many levels of sub-cracks may grow from it
    };

    /** Random walk around the heading; returns the points and spawns branches through the callback. */
    template <typename OnBranch>
    std::vector<juce::Point<float>> walk (const Crack& crack, juce::Random& random, OnBranch&& onBranch)
    {
        constexpr float step = 4.0f;
        std::vector<juce::Point<float>> points { crack.start };
        auto angle = crack.heading;

        for (float travelled = 0.0f; travelled < crack.length; travelled += step)
        {
            angle += (random.nextFloat() - 0.5f) * 0.55f;
            angle = angle * 0.85f + crack.heading * 0.15f;

            const auto next = points.back() + juce::Point<float> (std::cos (angle), std::sin (angle)) * step;
            points.push_back (next);

            if (crack.branches > 0 && travelled > 10.0f && random.nextFloat() < 0.07f)
            {
                const auto side = random.nextBool() ? 1.0f : -1.0f;
                onBranch (Crack { next,
                                  angle + side * (0.45f + random.nextFloat() * 0.5f),
                                  (crack.length - travelled) * (0.3f + random.nextFloat() * 0.3f),
                                  crack.width * 0.6f,
                                  crack.branches - 1 });
            }
        }

        return points;
    }

    void drawCrack (juce::Graphics& g, const Crack& crack, juce::Random& random)
    {
        std::vector<Crack> children;
        const auto points = walk (crack, random, [&children] (Crack child) { children.push_back (child); });
        const auto count = (float) (points.size() - 1);

        for (size_t i = 1; i < points.size(); ++i)
        {
            const auto t = (float) (i - 1) / count;
            const auto width = crack.width * (1.0f - 0.8f * t);
            const juce::Line<float> segment (points[i - 1], points[i]);

            g.setColour (colours::gold.withAlpha (0.06f));
            g.drawLine (segment, width * 4.0f);

            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawLine (segment, width * 1.8f);

            g.setColour ((t < 0.5f ? colours::goldLight : colours::gold).withAlpha (0.55f - 0.25f * t));
            g.drawLine (segment, width);
        }

        for (const auto& child : children)
            drawCrack (g, child, random);
    }
}

MarbleTexture::MarbleTexture()
    : image (createMarble())
{
}

void drawGoldCracks (juce::Graphics& g)
{
    juce::Random random (0x6011d);

    // Each crack starts at the enclosure edge and fades out towards the middle.
    const Crack cracks[] = {
        { { 8.0f, 296.0f },   0.32f,                                  150.0f, 1.3f, 2 },
        { { 432.0f, 548.0f }, juce::MathConstants<float>::pi + 0.42f, 125.0f, 1.2f, 2 },
        { { 318.0f, 8.0f },   1.95f,                                   70.0f, 1.0f, 1 },
        { { 70.0f, 652.0f },  -1.05f,                                  80.0f, 1.0f, 1 },
    };

    for (const auto& crack : cracks)
        drawCrack (g, crack, random);
}
}
