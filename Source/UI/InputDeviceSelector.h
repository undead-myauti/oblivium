#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace oblivium::ui
{
/** Drop-down listing the audio input devices of the current driver (standalone app only).
    The list is rescanned every time it opens, so newly plugged interfaces show up. */
class InputDeviceSelector final : public juce::ComboBox,
                                  private juce::ChangeListener
{
public:
    explicit InputDeviceSelector (juce::AudioDeviceManager&);
    ~InputDeviceSelector() override;

    void showPopup() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void refresh();
    void selectDevice (const juce::String& name);

    juce::AudioDeviceManager& deviceManager;
    juce::StringArray deviceNames;

    static constexpr int noInputId = 1000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputDeviceSelector)
};
}
