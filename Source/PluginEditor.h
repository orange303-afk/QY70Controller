#pragma once

#include <JuceHeader.h>

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "VoiceCatalog.h"

class QY70ControllerAudioProcessor;

// ─── Skeuomorphic hardware LookAndFeel ───────────────────────────────────────
class HardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Palette — silver-gray metallic (matches QY70 hardware body)
    static constexpr juce::uint32 panelBg    = 0xFFB6B8B4;  // warm silver base
    static constexpr juce::uint32 panelDark  = 0xFF8A8C88;  // shadow section
    static constexpr juce::uint32 panelLight = 0xFFDADCD8;  // bright highlight silver
    static constexpr juce::uint32 lcdBg      = 0xFF3A4428;  // LCD dark green bg
    static constexpr juce::uint32 lcdGlow    = 0xFF9AC847;  // LCD bright number
    static constexpr juce::uint32 lcdDim     = 0xFF6B8C34;  // LCD dim segment
    static constexpr juce::uint32 knobBody   = 0xFF373432;  // knob dark body
    static constexpr juce::uint32 knobRing   = 0xFF4B4844;  // knob ring
    static constexpr juce::uint32 btnDark    = 0xFF302D2A;  // dark metal button
    static constexpr juce::uint32 btnXgOff   = 0xFF6E2626;  // XG OFF red
    static constexpr juce::uint32 btnXgOn    = 0xFF266E41;  // XG ON green

    HardwareLookAndFeel();

    // Rotary knob: multi-layer 3-D render
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    // Slider text box: LCD style
    juce::Label* createSliderTextBox (juce::Slider&) override;

    // Shift textbox down 3px for visual depth gap
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;

    // Buttons: metallic bevel style
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    // Tabs: hardware panel style
    void drawTabButton (juce::TabBarButton&, juce::Graphics&,
                        bool isMouseOver, bool isMouseDown) override;

    juce::Colour getTabBackgroundColour (juce::TabbedComponent&);

    int getTabButtonBestWidth (juce::TabBarButton&, int) override;

    // Combo box: panel style
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    // Helper: draw a bevel/inset rectangle (used by several elements)
    static void drawBevel (juce::Graphics& g, juce::Rectangle<float> bounds,
                           float thickness, bool raised);

    // LCD labels: draw inner shadow after default fill
    void drawLabel (juce::Graphics&, juce::Label&) override;
};

class ParameterPage final : public juce::Component
{
public:
    ParameterPage(juce::AudioProcessorValueTreeState& state,
                  const juce::StringArray& parameterIds,
                  juce::String description = {},
                  juce::LookAndFeel* customLAF = nullptr);
    void setParameterLabel(const juce::String& parameterId, const juce::String& text);
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::Viewport viewport;
    juce::Component content;
    juce::Label descriptionLabel;
    juce::StringArray ids;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
};

// Simple component that paints a sunken rounded-rect background
// (used to visually group knob grids)
class SunkenPanel final : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        constexpr float corner = 8.0f;

        // Dark outer rim
        g.setColour (juce::Colour (0x60000000));
        g.fillRoundedRectangle (b, corner);

        // Fill: slightly darker than panel background
        auto inner = b.reduced (1.5f);
        g.setColour (juce::Colour (HardwareLookAndFeel::panelBg).darker (0.14f));
        g.fillRoundedRectangle (inner, corner - 1.0f);

        // Inner shadow — top
        juce::ColourGradient topShadow (
            juce::Colour (0x4A000000), inner.getX(), inner.getY(),
            juce::Colour (0x00000000), inner.getX(), inner.getY() + 14.0f, false);
        g.setGradientFill (topShadow);
        g.fillRoundedRectangle (inner, corner - 1.0f);

        // Inner shadow — left
        juce::ColourGradient leftShadow (
            juce::Colour (0x20000000), inner.getX(), inner.getY(),
            juce::Colour (0x00000000), inner.getX() + 10.0f, inner.getY(), false);
        g.setGradientFill (leftShadow);
        g.fillRoundedRectangle (inner, corner - 1.0f);

        // Bottom-right glint
        juce::ColourGradient br (
            juce::Colour (0x00FFFFFF), inner.getX(), inner.getBottom(),
            juce::Colour (0x14FFFFFF), inner.getRight(), inner.getBottom(), false);
        g.setGradientFill (br);
        g.fillRoundedRectangle (inner, corner - 1.0f);
    }
};

class ParameterStepper final : public juce::Component
{
public:
    ParameterStepper();

    juce::Slider& attachmentSlider() { return valueSlider; }
    void setDiscreteValues(std::vector<double> values);
    void setValueChangeCallback(std::function<void()> callback);
    void syncDisplayedValue();
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;

private:
    void stepBy(double amount);
    double nearestDiscreteValue(double value) const;

    juce::TextButton previousButton { juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton nextButton { juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::Label valueLabel;
    juce::Slider valueSlider;
    std::vector<double> discreteValues;
    std::function<void()> valueChangeCallback;
};

// Lightweight component that paints the LCD surround behind the steppers
class LcdPanel final : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced(0.5f);
        const float corner = 5.0f;

        // Outer inset shadow
        g.setColour(juce::Colour(0xFF181510));
        g.fillRoundedRectangle(b, corner);

        // Inner LCD background
        auto inner = b.reduced(2.0f);
        juce::ColourGradient bg(juce::Colour(0xFF3E4C2A), inner.getX(), inner.getY(),
                                juce::Colour(0xFF2E3820), inner.getRight(), inner.getBottom(), false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(inner, corner - 1.0f);

        // Glare reflection strip
        auto glare = inner.removeFromTop(inner.getHeight() * 0.35f);
        juce::ColourGradient glareGrad(juce::Colour(0x12FFFFFF), glare.getX(), glare.getY(),
                                       juce::Colour(0x00FFFFFF), glare.getX(), glare.getBottom(), false);
        g.setGradientFill(glareGrad);
        g.fillRoundedRectangle(glare, corner - 1.0f);

        // Inset border
        HardwareLookAndFeel::drawBevel(g, b, 1.5f, false);
    }
};

class QY70ControllerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer,
                                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit QY70ControllerAudioProcessorEditor(QY70ControllerAudioProcessor&);
    ~QY70ControllerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    QY70ControllerAudioProcessor& owner;
    void refreshXgButton();
    void updateVoiceChoices(bool bankChanged, bool resetLsb);
    void updateVoiceName();
    void showVoiceMenu();
    void selectVoice(const qy70::VoiceDescriptor& voice);
    void stepCatalogVoice(int direction);
    void layoutVoicePage();
    void timerCallback() override;
    void updateEffectLabels();

    // Per-part state recall
    // IDs of parameters that are saved/restored per part (voice + core mix params)
    static const juce::StringArray& partSaveIds();
    void saveCurrentPartState();
    void restorePartState(int part);
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    int lastKnownPart = -1;
    // perPartState[part-1][paramId] = normalised 0..1 value
    std::array<std::map<juce::String, float>, 16> perPartState;

    juce::ImageComponent logoComponent;
    juce::TextButton voicePreviousButton { juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton voiceNameButton;
    juce::TextButton voiceNextButton { juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::TextButton xgButton { "XG OFF" };
    juce::TextButton fetchButton { "Fetch Part" };
    juce::TextButton sendButton { "Send Snapshot" };
    juce::TextButton resetButton { "Reset Edits" };
    juce::TextButton settingsButton { "Settings" };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Component voicePage;
    LcdPanel lcdPanel;
    SunkenPanel knobGroupPanel;
    std::unique_ptr<ParameterPage> partPage;
    std::unique_ptr<ParameterPage> controllerPage;
    std::unique_ptr<ParameterPage> pitchPage;
    std::unique_ptr<ParameterPage> reverbPage;
    std::unique_ptr<ParameterPage> chorusPage;
    std::unique_ptr<ParameterPage> variationPage;
    std::unique_ptr<juce::Component> settingsOverlay;
    std::array<ParameterStepper, 4> steppers;
    std::array<juce::Slider, 9> knobSliders;
    std::array<juce::Label, 13> parameterLabels;
    std::array<std::unique_ptr<SliderAttachment>, 13> attachments;
    std::array<juce::String, 13> parameterIds;
    bool updatingVoiceChoices = false;

    HardwareLookAndFeel hardwareLAF;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessorEditor)
};
