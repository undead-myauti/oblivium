#include "InputDeviceSelector.h"

namespace oblivium::ui
{
namespace
{
    // ALSA also advertises resamplers, channel mixers and output-only streams as inputs;
    // none of them is a microphone. They stay reachable from the standalone's audio settings.
    bool isUsefulInput (const juce::String& name)
    {
        for (const auto* noise : { "Rate Converter Plugin", "Plugin for channel", "Plugin using Speex",
                                   "USB Stream Output", "Open Sound System" })
            if (name.contains (noise))
                return false;

        return true;
    }

    // Turns ALSA's descriptions into something readable. Other drivers' names are kept as they are.
    juce::String displayName (const juce::String& name)
    {
        if (name.startsWith ("Default ALSA"))
            return "System default";

        return name.replace ("; Direct hardware device without any conversions", " (direct)")
                   .replace ("; Direct sample snooping device", " (shared)")
                   .replace ("; Front output / input", "");
    }
}

InputDeviceSelector::InputDeviceSelector (juce::AudioDeviceManager& manager)
    : deviceManager (manager)
{
    setTitle ("Input device");
    setTooltip ("Audio input device");
    setTextWhenNothingSelected ("Select input device");
    setTextWhenNoChoicesAvailable ("No input devices");
    setJustificationType (juce::Justification::centred);

    onChange = [this]
    {
        const auto id = getSelectedId();
        if (id == noInputId)
            selectDevice ({});
        else if (juce::isPositiveAndBelow (id - 1, deviceNames.size()))
            selectDevice (deviceNames[id - 1]);
    };

    deviceManager.addChangeListener (this);

    if (auto* type = deviceManager.getCurrentDeviceTypeObject())
        type->scanForDevices();

    refresh();
}

InputDeviceSelector::~InputDeviceSelector()
{
    deviceManager.removeChangeListener (this);
}

void InputDeviceSelector::showPopup()
{
    if (auto* type = deviceManager.getCurrentDeviceTypeObject())
        type->scanForDevices();

    refresh();
    juce::ComboBox::showPopup();
}

void InputDeviceSelector::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refresh();
}

void InputDeviceSelector::refresh()
{
    clear (juce::dontSendNotification);
    deviceNames.clear();

    auto* type = deviceManager.getCurrentDeviceTypeObject();
    if (type == nullptr)
        return;

    const auto current = deviceManager.getAudioDeviceSetup().inputDeviceName;

    for (const auto& name : type->getDeviceNames (true))
        if (isUsefulInput (name) || name == current)
            deviceNames.add (name);

    for (int i = 0; i < deviceNames.size(); ++i)
        addItem (displayName (deviceNames[i]), i + 1);

    addSeparator();
    addItem ("No input", noInputId);

    const auto index = deviceNames.indexOf (current);
    setSelectedId (current.isEmpty() ? noInputId : index + 1, juce::dontSendNotification);
}

void InputDeviceSelector::selectDevice (const juce::String& name)
{
    auto setup = deviceManager.getAudioDeviceSetup();

    if (setup.inputDeviceName == name)
        return;

    setup.inputDeviceName = name;
    setup.useDefaultInputChannels = true;

    // Drivers such as ASIO open input and output as one device.
    if (auto* type = deviceManager.getCurrentDeviceTypeObject())
        if (! type->hasSeparateInputsAndOutputs() && name.isNotEmpty())
            setup.outputDeviceName = name;

    const auto error = deviceManager.setAudioDeviceSetup (setup, true);

    if (error.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                "Audio input", "Could not open \"" + name + "\":\n" + error);

    refresh();
}
}
