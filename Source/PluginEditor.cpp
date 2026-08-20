#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "QY70Midi.h"
#include "VoiceCatalog.h"
#include "LogoData.h"

#if JUCE_STANDALONE_APPLICATION
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

#include <algorithm>
#include <cmath>
#include <iterator>

// ═══════════════════════════════════════════════════════════════════════════════
// HardwareLookAndFeel — skeuomorphic hardware-unit style
// ═══════════════════════════════════════════════════════════════════════════════

HardwareLookAndFeel::HardwareLookAndFeel()
{
    const auto bg = juce::Colour (panelBg);
    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::Label::textColourId,  juce::Colour (0xFF1A1710));
    setColour (juce::Slider::textBoxTextColourId,  juce::Colour (lcdGlow));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (lcdBg));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xFF222B14));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (lcdGlow));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF1A1A1A));
    // Buttons: same dark gray as knob body
    setColour (juce::TextButton::buttonColourId,  juce::Colour (knobBody));
    setColour (juce::TextButton::textColourOffId, juce::Colour (panelLight));
    setColour (juce::TextButton::textColourOnId,  juce::Colour (panelLight));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (lcdBg));
    setColour (juce::ComboBox::textColourId,       juce::Colour (lcdGlow));
    setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xFF222B14));
    setColour (juce::ComboBox::arrowColourId,      juce::Colour (lcdGlow));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (lcdBg));
    setColour (juce::PopupMenu::textColourId,       juce::Colour (lcdGlow));
    setColour (juce::PopupMenu::headerTextColourId, juce::Colour (lcdGlow));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFF4A5A30));
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colour (0xFFC8F060));
    setColour (juce::ScrollBar::thumbColourId, juce::Colour (panelDark));
    setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colour (0xFF888070));
}

// ── Helper: bevel rectangle ───────────────────────────────────────────────────
void HardwareLookAndFeel::drawBevel (juce::Graphics& g,
                                     juce::Rectangle<float> b,
                                     float t, bool raised)
{
    const auto hi  = raised ? juce::Colour (0x55FFFFFF) : juce::Colour (0x55000000);
    const auto lo  = raised ? juce::Colour (0x55000000) : juce::Colour (0x55FFFFFF);
    // top + left
    g.setColour (hi);
    g.fillRect (b.getX(), b.getY(), b.getWidth(), t);
    g.fillRect (b.getX(), b.getY(), t, b.getHeight());
    // bottom + right
    g.setColour (lo);
    g.fillRect (b.getX(), b.getBottom() - t, b.getWidth(), t);
    g.fillRect (b.getRight() - t, b.getY(), t, b.getHeight());
}

// ── Rotary knob: flat hardware style (no ball/sphere effect) ─────────────────
void HardwareLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                            int x, int y, int width, int height,
                                            float pos,
                                            float startAngle, float endAngle,
                                            juce::Slider& slider)
{
    juce::ignoreUnused (slider);
    const float cx = (float)x + (float)width  * 0.5f;
    const float cy = (float)y + (float)height * 0.5f;
    const float radius = juce::jmin ((float)width, (float)height) * 0.5f - 4.0f;
    if (radius < 4.0f) return;

    // ── 1. Drop shadow ─────────────────────────────────────────────────────
    g.setColour (juce::Colour (0x44000000));
    g.fillEllipse (cx - radius + 2.0f, cy - radius + 4.0f,
                   radius * 2.0f, radius * 2.0f);

    // ── 2. Arc track + active arc ──────────────────────────────────────────
    {
        const float trackR = radius + 2.5f;
        juce::Path track;
        track.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xFF141414));
        g.strokePath (track, juce::PathStrokeType (3.5f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        const float curAngle = startAngle + pos * (endAngle - startAngle);
        if (curAngle > startAngle)
        {
            juce::Path active;
            active.addCentredArc (cx, cy, trackR, trackR, 0.0f, startAngle, curAngle, true);
            g.setColour (juce::Colour (lcdGlow).withAlpha (0.78f));
            g.strokePath (active, juce::PathStrokeType (3.5f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }
    }

    // ── 3. Outer rim: vertical linear gradient (cylindrical, not spherical) ─
    {
        juce::ColourGradient rimGrad (
            juce::Colour (0xFF686360), cx, cy - radius,   // top: lighter
            juce::Colour (0xFF282624), cx, cy + radius,   // bottom: darker
            false);
        rimGrad.addColour (0.5, juce::Colour (0xFF4A4744));
        g.setGradientFill (rimGrad);
        g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }

    // ── 4. Inner knob body: flat top, linear gradient top→bottom ──────────
    {
        const float innerR = radius * 0.80f;
        // Vertical linear gradient — flat surface look, no ball effect
        juce::ColourGradient bodyGrad (
            juce::Colour (0xFF484440), cx, cy - innerR,   // top: slightly lighter
            juce::Colour (0xFF1E1C1A), cx, cy + innerR,   // bottom: darker edge
            false);
        bodyGrad.addColour (0.4, juce::Colour (0xFF373432));  // mid: base knob gray
        g.setGradientFill (bodyGrad);
        g.fillEllipse (cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

        // Very thin chamfer highlight at the very top edge (not a blob)
        g.setColour (juce::Colour (0x20FFFFFF));
        g.drawEllipse (cx - innerR + 0.5f, cy - innerR + 0.5f,
                       innerR * 2.0f - 1.0f, innerR * 2.0f - 1.0f, 1.0f);
    }

    // ── 5. Indicator line: white, starting from center, reaching near edge ─
    {
        const float angle = startAngle + pos * (endAngle - startAngle)
                            - juce::MathConstants<float>::halfPi;
        const float innerR = radius * 0.80f;
        const float x1 = cx + std::cos (angle) * (innerR * 0.20f);
        const float y1 = cy + std::sin (angle) * (innerR * 0.20f);
        const float x2 = cx + std::cos (angle) * (innerR * 0.82f);
        const float y2 = cy + std::sin (angle) * (innerR * 0.82f);
        g.setColour (juce::Colours::white.withAlpha (0.93f));
        g.drawLine (x1, y1, x2, y2, 2.4f);
    }
}


// ── LCD-style slider text box ─────────────────────────────────────────────────
juce::Label* HardwareLookAndFeel::createSliderTextBox (juce::Slider&)
{
    auto* label = new juce::Label();
    label->setJustificationType (juce::Justification::centred);
    label->setColour (juce::Label::backgroundColourId, juce::Colour (lcdBg));
    label->setColour (juce::Label::outlineColourId,    juce::Colour (0xFF080C04));
    label->setColour (juce::Label::textColourId,       juce::Colour (lcdGlow));
    label->setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f,
                                       juce::Font::plain));
    return label;
}

// ── Slider layout: shift textbox 3px down for depth gap ────────────────────
juce::Slider::SliderLayout HardwareLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    auto layout = juce::LookAndFeel_V4::getSliderLayout (slider);
    layout.textBoxBounds = layout.textBoxBounds.translated (0, 3);
    return layout;
}

// ── Draw LCD inner-shadow over a component's label (reusable) ─────────────────
static void drawLcdInnerShadow (juce::Graphics& g, juce::Rectangle<float> bounds, float corner = 3.0f)
{
    // Top shadow band
    juce::ColourGradient topShadow (
        juce::Colour (0x60000000), bounds.getX(), bounds.getY(),
        juce::Colour (0x00000000), bounds.getX(), bounds.getY() + 6.0f, false);
    g.setGradientFill (topShadow);
    g.fillRoundedRectangle (bounds, corner);

    // Left shadow band
    juce::ColourGradient leftShadow (
        juce::Colour (0x30000000), bounds.getX(), bounds.getY(),
        juce::Colour (0x00000000), bounds.getX() + 5.0f, bounds.getY(), false);
    g.setGradientFill (leftShadow);
    g.fillRoundedRectangle (bounds, corner);
}


// ── Metallic button background ────────────────────────────────────────────────
void HardwareLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                                juce::Button& button,
                                                const juce::Colour& bgColour,
                                                bool isHighlighted,
                                                bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const float cornerSize = 4.0f;

    auto col = bgColour;
    if (isHighlighted) col = col.brighter (0.10f);
    if (isDown)        col = col.darker   (0.15f);

    // Detect LCD-style buttons (voice name display) by their dark green hue
    const bool isLcdStyle = col.getGreen() > col.getRed()
                            && col.getBrightness() < 0.22f;

    if (isLcdStyle)
    {
        // Inset / sunken look: dark outer border, inset shadow on top/left
        g.setColour (juce::Colour (0xFF0E1208));
        g.fillRoundedRectangle (bounds, cornerSize);

        auto face = bounds.reduced (1.5f);
        g.setColour (col);
        g.fillRoundedRectangle (face, cornerSize - 1.0f);

        // Inset bevel (shadow at top/left, highlight at bottom/right)
        drawBevel (g, face, 1.2f, false);

        // Gradient inner shadow from top
        drawLcdInnerShadow (g, face, cornerSize - 1.0f);
    }
    else
    {
        // Raised / metallic look
        g.setColour (juce::Colour (0xFF181614));
        g.fillRoundedRectangle (bounds, cornerSize);

        auto face = bounds.reduced (1.5f);
        juce::ColourGradient faceGrad (col.brighter (isDown ? 0.0f : 0.08f),
                                       face.getX(), face.getY(),
                                       col.darker (0.15f),
                                       face.getX(), face.getBottom(), false);
        g.setGradientFill (faceGrad);
        g.fillRoundedRectangle (face, cornerSize - 1.0f);

        drawBevel (g, face, 1.2f, !isDown);
    }
}


void HardwareLookAndFeel::drawButtonText (juce::Graphics& g,
                                          juce::TextButton& button,
                                          bool /*isHighlighted*/,
                                          bool isDown)
{
    const float yOffset = isDown ? 1.0f : 0.0f;
    const auto text = button.getButtonText();
    const auto textColour = button.findColour (button.getToggleState()
                                               ? juce::TextButton::textColourOnId
                                               : juce::TextButton::textColourOffId);
    g.setColour (textColour.isOpaque() ? textColour : juce::Colour (0xFFDAD5CB));

    // If button text ends with the down-arrow glyph (▼), draw main text centred
    // and the arrow right-aligned so it acts as a dropdown indicator
    const juce::String arrow = juce::String::fromUTF8 ("\xe2\x96\xbc");
    if (text.endsWith (arrow))
    {
        const auto mainText = text.dropLastCharacters (arrow.length());
        const auto bounds = button.getLocalBounds().translated (0, (int)yOffset);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (mainText, bounds.reduced (36, 0), juce::Justification::centred, true);
        g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
        g.drawText (arrow, bounds.reduced (6, 0), juce::Justification::centredRight, false);
    }
    else
    {
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (text,
                    button.getLocalBounds().translated (0, (int)yOffset),
                    juce::Justification::centred, true);
    }
}

// ── LCD label inner shadow ────────────────────────────────────────────────────
void HardwareLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    // Call default rendering first
    juce::LookAndFeel_V4::drawLabel (g, label);

    // Add inner shadow only to LCD-coloured labels (dark green background)
    const auto bgCol = label.findColour (juce::Label::backgroundColourId);
    if (bgCol.getAlpha() > 0
        && bgCol.getGreen() > bgCol.getRed()
        && bgCol.getBrightness() < 0.30f)
    {
        const auto bounds = label.getLocalBounds().toFloat().reduced (1.0f);
        drawLcdInnerShadow (g, bounds, 3.0f);
    }
}


// ── Tabs ──────────────────────────────────────────────────────────────────────
int HardwareLookAndFeel::getTabButtonBestWidth (juce::TabBarButton& btn, int depth)
{
    juce::ignoreUnused (depth);
    // ~8px per character + 26px padding
    return btn.getButtonText().length() * 8 + 26;
}

void HardwareLookAndFeel::drawTabButton (juce::TabBarButton& button,
                                         juce::Graphics& g,
                                         bool isMouseOver, bool isMouseDown)
{
    const bool isActive = button.getToggleState();
    auto bounds = button.getLocalBounds().toFloat();

    const auto bgCol  = isActive  ? juce::Colour (panelBg)
                      : isMouseOver ? juce::Colour (panelDark).brighter (0.1f)
                                    : juce::Colour (panelDark);
    g.setColour (bgCol);
    g.fillRect (bounds);

    // Top highlight for active tab
    if (isActive)
    {
        g.setColour (juce::Colour (0xFF5A7A9A));  // cool steel-blue accent for active tab
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 2.0f);
    }

    // Separator line on right
    g.setColour (juce::Colour (0xFF787878));
    g.fillRect (bounds.getRight() - 1.0f, bounds.getY() + 4.0f, 1.0f, bounds.getHeight() - 4.0f);

    juce::ignoreUnused (isMouseDown);

    // Tab label
    const auto textCol = isActive ? juce::Colour (0xFF1A1A1A)
                                  : juce::Colour (0xFF3A3A3A);
    g.setColour (textCol);
    g.setFont (juce::FontOptions (12.0f, isActive ? juce::Font::bold : juce::Font::plain));
    g.drawText (button.getButtonText(), bounds.reduced (6, 2),
                juce::Justification::centred, true);
}

juce::Colour HardwareLookAndFeel::getTabBackgroundColour (juce::TabbedComponent&)
{
    return juce::Colour (panelBg);
}

// ── ComboBox ──────────────────────────────────────────────────────────────────
void HardwareLookAndFeel::drawComboBox (juce::Graphics& g,
                                        int width, int height,
                                        bool /*isButtonDown*/,
                                        int buttonX, int buttonY,
                                        int buttonW, int buttonH,
                                        juce::ComboBox& box)
{
    juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH);
    const auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
    const float corner = 3.0f;

    g.setColour (juce::Colour (lcdBg));
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (juce::Colour (0xFF222B14));
    g.drawRoundedRectangle (bounds, corner, 1.2f);

    // Inner shadow (sunken LCD look)
    drawLcdInnerShadow (g, bounds.reduced (1.0f), corner);

    // Arrow
    const float arrowX = (float)(width - 18);
    const float arrowY = (float)(height / 2) - 3.0f;
    g.setColour (juce::Colour (lcdGlow));
    juce::Path arrow;
    arrow.addTriangle (arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f, arrowY + 6.0f);
    g.fillPath (arrow);

    juce::ignoreUnused (box);
}

ParameterPage::ParameterPage(juce::AudioProcessorValueTreeState& state,
                             const juce::StringArray& parameterIds,
                             juce::String description,
                             juce::LookAndFeel* customLAF)
{
    if (customLAF != nullptr)
        setLookAndFeel(customLAF);

    viewport.setScrollBarsShown(true, false);
    viewport.setOpaque(false);  // allow parent paint to show through
    viewport.setViewedComponent(&content, false);
    addAndMakeVisible(viewport);

    descriptionLabel.setText(std::move(description), juce::dontSendNotification);
    descriptionLabel.setJustificationType(juce::Justification::centredLeft);
    descriptionLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF1E1E1E));
    descriptionLabel.setMinimumHorizontalScale(0.8f);
    content.addAndMakeVisible(descriptionLabel);

    for (const auto& id : parameterIds)
    {
        auto* parameter = state.getParameter(id);
        if (parameter == nullptr)
            continue;

        ids.add(id);

        auto label = std::make_unique<juce::Label>();
        label->setText(parameter->getName(96), juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colour(0xFF1A1A1A));
        content.addAndMakeVisible(*label);
        labels.push_back(std::move(label));

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(parameter))
        {
            auto combo = std::make_unique<juce::ComboBox>();
            if (customLAF != nullptr)
                combo->setLookAndFeel(customLAF);
            combo->addItemList(choice->choices, 1);
            combo->setJustificationType(juce::Justification::centred);
            content.addAndMakeVisible(*combo);
            comboAttachments.push_back(std::make_unique<ComboAttachment>(state, id, *combo));
            controls.push_back(std::move(combo));
        }
        else if (dynamic_cast<juce::AudioParameterBool*>(parameter) != nullptr)
        {
            // Render bool parameters as XG-style toggle TextButton with ON/OFF text
            auto button = std::make_unique<juce::TextButton>("OFF");
            if (customLAF != nullptr)
                button->setLookAndFeel(customLAF);
            button->setClickingTogglesState(true);
            button->setColour(juce::TextButton::buttonColourId,  juce::Colour(HardwareLookAndFeel::knobBody));
            button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(HardwareLookAndFeel::btnXgOn));
            button->setColour(juce::TextButton::textColourOffId, juce::Colour(HardwareLookAndFeel::panelLight));
            button->setColour(juce::TextButton::textColourOnId,  juce::Colour(HardwareLookAndFeel::panelLight));
            button->onStateChange = [btn = button.get()] {
                btn->setButtonText(btn->getToggleState() ? "ON" : "OFF");
            };
            content.addAndMakeVisible(*button);
            buttonAttachments.push_back(std::make_unique<ButtonAttachment>(state, id, *button));
            // Sync text after attachment sets initial toggle state
            button->setButtonText(button->getToggleState() ? "ON" : "OFF");
            controls.push_back(std::move(button));
        }
        else
        {
            auto slider = std::make_unique<juce::Slider>();
            if (customLAF != nullptr)
                slider->setLookAndFeel(customLAF);
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 22);
            slider->setDoubleClickReturnValue(
                true,
                state.getParameterRange(id).convertFrom0to1(parameter->getDefaultValue()));
            content.addAndMakeVisible(*slider);
            sliderAttachments.push_back(std::make_unique<SliderAttachment>(state, id, *slider));
            controls.push_back(std::move(slider));
        }
    }
}

void ParameterPage::setParameterLabel(const juce::String& parameterId,
                                      const juce::String& text)
{
    const auto index = ids.indexOf(parameterId);
    if (juce::isPositiveAndBelow(index, static_cast<int>(labels.size())))
        labels[static_cast<std::size_t>(index)]->setText(text, juce::dontSendNotification);
}

void ParameterPage::resized()
{
    viewport.setBounds(getLocalBounds());
    const auto contentWidth = juce::jmax(400, viewport.getWidth() - 14);
    const auto columns = contentWidth >= 900 ? 5 : (contentWidth >= 680 ? 4 : 3);
    constexpr int cellHeight = 126;
    const auto top = descriptionLabel.getText().isNotEmpty() ? 48 : 8;
    const auto rows = (static_cast<int>(controls.size()) + columns - 1) / columns;
    content.setSize(contentWidth, top + rows * cellHeight + 16);
    descriptionLabel.setBounds(18, 6, contentWidth - 36, top - 8);

    const auto cellWidth = contentWidth / columns;
    for (std::size_t index = 0; index < controls.size(); ++index)
    {
        const auto column = static_cast<int>(index) % columns;
        const auto row = static_cast<int>(index) / columns;
        auto cell = juce::Rectangle<int>(column * cellWidth,
                                         top + row * cellHeight,
                                         cellWidth,
                                         cellHeight).reduced(8, 3);
        labels[index]->setBounds(cell.removeFromTop(25));

        if (dynamic_cast<juce::ComboBox*>(controls[index].get()) != nullptr)
            controls[index]->setBounds(cell.removeFromTop(38).reduced(5, 3));
        else if (dynamic_cast<juce::TextButton*>(controls[index].get()) != nullptr)
            controls[index]->setBounds(cell.removeFromTop(38).withSizeKeepingCentre(110, 28));
        else
            controls[index]->setBounds(cell.reduced(8, 0));
    }
}

void ParameterPage::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (4.0f);
    constexpr float corner = 8.0f;

    // Dark outer rim (sunken look)
    g.setColour (juce::Colour (0x50000000));
    g.fillRoundedRectangle (b, corner);

    // Fill: slightly darker than panel
    auto inner = b.reduced (1.5f);
    g.setColour (juce::Colour (HardwareLookAndFeel::panelBg).darker (0.12f));
    g.fillRoundedRectangle (inner, corner - 1.0f);

    // Inner shadow — top
    juce::ColourGradient topShadow (
        juce::Colour (0x44000000), inner.getX(), inner.getY(),
        juce::Colour (0x00000000), inner.getX(), inner.getY() + 16.0f, false);
    g.setGradientFill (topShadow);
    g.fillRoundedRectangle (inner, corner - 1.0f);

    // Inner shadow — left
    juce::ColourGradient leftShadow (
        juce::Colour (0x1C000000), inner.getX(), inner.getY(),
        juce::Colour (0x00000000), inner.getX() + 12.0f, inner.getY(), false);
    g.setGradientFill (leftShadow);
    g.fillRoundedRectangle (inner, corner - 1.0f);
}

ParameterStepper::ParameterStepper()
{
    valueSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    valueSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    valueSlider.onValueChange = [this]
    {
        syncDisplayedValue();
        if (valueChangeCallback)
            valueChangeCallback();
    };

    previousButton.setTooltip("Previous value");
    nextButton.setTooltip("Next value");
    previousButton.setRepeatSpeed(400, 90, 20);
    nextButton.setRepeatSpeed(400, 90, 20);
    previousButton.onClick = [this] { stepBy(-1.0); };
    nextButton.onClick = [this] { stepBy(1.0); };

    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setEditable(true, true, false);
    valueLabel.setColour(juce::Label::backgroundColourId, juce::Colour(HardwareLookAndFeel::lcdBg));
    valueLabel.setColour(juce::Label::outlineColourId,    juce::Colour(0xFF1A2210));
    valueLabel.setColour(juce::Label::textColourId,       juce::Colour(HardwareLookAndFeel::lcdGlow));
    valueLabel.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 16.0f,
                                         juce::Font::bold));
    valueLabel.onTextChange = [this]
    {
        valueSlider.setValue(nearestDiscreteValue(valueLabel.getText().getDoubleValue()),
                             juce::sendNotificationSync);
        syncDisplayedValue();
    };
    valueLabel.addMouseListener(this, false);

    addAndMakeVisible(previousButton);
    addAndMakeVisible(valueLabel);
    addAndMakeVisible(nextButton);
}

void ParameterStepper::stepBy(double amount)
{
    if (!discreteValues.empty())
    {
        const auto current = nearestDiscreteValue(valueSlider.getValue());
        const auto iterator = std::lower_bound(discreteValues.begin(), discreteValues.end(), current);
        const auto index = static_cast<int>(std::distance(discreteValues.begin(), iterator));
        const auto nextIndex = juce::jlimit(0,
                                            static_cast<int>(discreteValues.size()) - 1,
                                            index + (amount < 0.0 ? -1 : 1));
        valueSlider.setValue(discreteValues[static_cast<std::size_t>(nextIndex)],
                             juce::sendNotificationSync);
        return;
    }

    valueSlider.setValue(valueSlider.getValue() + amount, juce::sendNotificationSync);
}

double ParameterStepper::nearestDiscreteValue(double value) const
{
    if (discreteValues.empty())
        return value;

    return *std::min_element(discreteValues.begin(), discreteValues.end(),
                             [value](double left, double right)
                             {
                                 return std::abs(left - value) < std::abs(right - value);
                             });
}

void ParameterStepper::setDiscreteValues(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    discreteValues = std::move(values);
    valueSlider.setValue(nearestDiscreteValue(valueSlider.getValue()),
                         juce::sendNotificationSync);
}

void ParameterStepper::setValueChangeCallback(std::function<void()> callback)
{
    valueChangeCallback = std::move(callback);
}

void ParameterStepper::mouseWheelMove(const juce::MouseEvent&,
                                      const juce::MouseWheelDetails& wheel)
{
    const auto delta = wheel.deltaY != 0.0f ? wheel.deltaY : -wheel.deltaX;
    if (delta != 0.0f)
        stepBy(delta > 0.0f ? 1.0 : -1.0);
}

void ParameterStepper::syncDisplayedValue()
{
    valueLabel.setText(juce::String(juce::roundToInt(valueSlider.getValue())),
                       juce::dontSendNotification);
}

void ParameterStepper::resized()
{
    auto area = getLocalBounds();
    const auto buttonWidth = juce::jmin(38, area.getHeight());
    previousButton.setBounds(area.removeFromLeft(buttonWidth));
    nextButton.setBounds(area.removeFromRight(buttonWidth));
    valueLabel.setBounds(area.reduced(5, 0));
}

// ═══════════════════════════════════════════════════════════════════════════════
// SettingsOverlay — modal popup window for audio routing & settings
// ═══════════════════════════════════════════════════════════════════════════════

class SettingsOverlay final : public juce::Component
{
public:
    explicit SettingsOverlay(QY70ControllerAudioProcessor& processor)
        : audioProcessor(processor)
    {
        setOpaque(false);

        // Override text colors for SettingsOverlay children
        setColour(juce::Label::textColourId, juce::Colour(0xFFEBE7DC));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF161513));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xFFEBE7DC));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF4A453C));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xFFD0C8B8));

        titleLabel.setText("QY70 Controller  —  Settings", juce::dontSendNotification);
        titleLabel.setFont(juce::FontOptions(17.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFF5F2E8));
        addAndMakeVisible(titleLabel);

        midiOutLabel.setText("Hardware MIDI Output Device:", juce::dontSendNotification);
        midiOutLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        addAndMakeVisible(midiOutLabel);

        midiOutCombo.addItem("Host DAW MIDI Output (Default)", 1);
        const auto midiDevices = juce::MidiOutput::getAvailableDevices();
        int itemId = 2;
        int selectedIndex = 1;
        const auto currentDevId = audioProcessor.getHardwareMidiOutputDeviceId();

        for (const auto& dev : midiDevices)
        {
            midiOutCombo.addItem(dev.name, itemId);
            if (currentDevId == dev.identifier)
                selectedIndex = itemId;
            itemId++;
        }
        midiOutCombo.setSelectedId(selectedIndex, juce::dontSendNotification);

        midiOutCombo.onChange = [this, midiDevices] {
            const int id = midiOutCombo.getSelectedId();
            if (id <= 1)
            {
                audioProcessor.setHardwareMidiOutputDevice("");
            }
            else
            {
                const int devIndex = id - 2;
                if (juce::isPositiveAndBelow(devIndex, midiDevices.size()))
                {
                    audioProcessor.setHardwareMidiOutputDevice(midiDevices[devIndex].identifier);
                }
            }
        };
        addAndMakeVisible(midiOutCombo);

        // Clickable link labels instead of plain text
        link1.setText("ilyaorange.gumroad.com", juce::dontSendNotification);
        link2.setText("ilyaorange.bandcamp.com", juce::dontSendNotification);
        link3.setText("naukograd.bandcamp.com", juce::dontSendNotification);
        link4.setText("by Ilya Orange  |  Yamaha QY70 Controller", juce::dontSendNotification);

        for (auto* lk : { &link1, &link2, &link3 })
        {
            lk->setFont(juce::FontOptions(12.5f, juce::Font::bold));
            lk->setJustificationType(juce::Justification::centred);
            lk->setColour(juce::Label::textColourId, juce::Colour(0xFF7ABEFF));
            addAndMakeVisible(*lk);
        }
        link4.setFont(juce::FontOptions(12.0f, juce::Font::italic));
        link4.setJustificationType(juce::Justification::centred);
        link4.setColour(juce::Label::textColourId, juce::Colour(0xFFD4CEBD));
        addAndMakeVisible(link4);

        link1.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        link2.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        link3.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        link1.addMouseListener(this, false);
        link2.addMouseListener(this, false);
        link3.addMouseListener(this, false);

        closeButton.setButtonText("Close Settings");
        closeButton.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeButton);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        auto openUrl = [](const juce::String& url) {
            juce::URL(url).launchInDefaultBrowser();
        };
        if (e.eventComponent == &link1)
            openUrl("https://ilyaorange.gumroad.com");
        else if (e.eventComponent == &link2)
            openUrl("https://ilyaorange.bandcamp.com");
        else if (e.eventComponent == &link3)
            openUrl("https://naukograd.bandcamp.com");
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xD8080706)); // dark modal dimming backdrop

        constexpr int dialogWidth = 520;
        constexpr int dialogHeight = 310;

        auto bounds = getLocalBounds().withSizeKeepingCentre(dialogWidth, dialogHeight).toFloat();

        // Main modal dialog panel background (dark hardware casing)
        g.setColour(juce::Colour(0xFF22201D));
        g.fillRoundedRectangle(bounds, 10.0f);

        // Header bar inside dialog
        auto header = bounds.removeFromTop(38.0f);
        g.setColour(juce::Colour(0xFF2E2B27));
        g.fillRoundedRectangle(header, 10.0f);

        // Sub-panel background for MIDI Out controls
        auto midiCard = bounds.removeFromTop(48.0f).reduced(12.0f, 4.0f);
        g.setColour(juce::Colour(0xFF181715));
        g.fillRoundedRectangle(midiCard, 6.0f);
        g.setColour(juce::Colour(0xFF383530));
        g.drawRoundedRectangle(midiCard, 6.0f, 1.0f);

        // Outer border
        g.setColour(juce::Colour(0xFF5A544A));
        g.drawRoundedRectangle(getLocalBounds().withSizeKeepingCentre(dialogWidth, dialogHeight).toFloat(), 10.0f, 1.8f);
    }

    void resized() override
    {
        constexpr int dialogWidth = 520;
        constexpr int dialogHeight = 310;

        auto bounds = getLocalBounds().withSizeKeepingCentre(dialogWidth, dialogHeight);
        bounds.removeFromTop(6);

        auto topBar = bounds.removeFromTop(32);
        titleLabel.setBounds(topBar.reduced(14, 0));

        auto inner = bounds.reduced(20, 10);

        // MIDI Out Selector Card — label left, combo right, vertically centred
        auto midiCardArea = inner.removeFromTop(52);
        auto midiRow = midiCardArea.withSizeKeepingCentre(midiCardArea.getWidth(), 28);
        midiOutLabel.setBounds(midiRow.removeFromLeft(190));
        midiOutCombo.setBounds(midiRow.reduced(6, 0));

        inner.removeFromTop(8);

        // Links
        link4.setBounds(inner.removeFromTop(22));
        inner.removeFromTop(4);
        link1.setBounds(inner.removeFromTop(20));
        link2.setBounds(inner.removeFromTop(20));
        link3.setBounds(inner.removeFromTop(20));

        auto bottomRow = inner.removeFromBottom(30);
        closeButton.setBounds(bottomRow.removeFromRight(110));
    }

private:
    QY70ControllerAudioProcessor& audioProcessor;
    juce::Label titleLabel;
    juce::Label midiOutLabel;
    juce::ComboBox midiOutCombo;
    juce::Label link4, link1, link2, link3;
    juce::TextButton closeButton;
};

QY70ControllerAudioProcessorEditor::QY70ControllerAudioProcessorEditor(
    QY70ControllerAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      owner(processorToUse),
      parameterIds { "part", "bankMsb", "program", "bankLsb",
                     "volume", "pan", "cutoff", "resonance",
                     "attack", "release", "chorus", "reverb", "variation" }
{
    // ── Wire global LookAndFeel ──────────────────────────────────────────
    setLookAndFeel (&hardwareLAF);

    auto logoImage = juce::PNGImageFormat::loadFrom(logoPngData, logoPngDataSize);
    logoComponent.setImage(logoImage, juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid | juce::RectanglePlacement::onlyReduceInSize);
    addAndMakeVisible(logoComponent);

    // Voice name display: inset LCD style, darker than steppers
    voiceNameButton.setColour(juce::TextButton::buttonColourId,
                              juce::Colour(0xFF28321A));   // deeper/darker than lcdBg
    voiceNameButton.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour(0xFF2E3A1E));
    voiceNameButton.setColour(juce::TextButton::textColourOffId,
                              juce::Colour(HardwareLookAndFeel::lcdGlow));
    voiceNameButton.setColour(juce::TextButton::textColourOnId,
                              juce::Colour(HardwareLookAndFeel::lcdGlow).brighter(0.1f));
    voiceNameButton.setTooltip("Open the categorized QY70 voice browser");
    voiceNameButton.onClick = [this] { showVoiceMenu(); };

    voicePreviousButton.setTooltip("Previous voice in the catalog");
    voiceNextButton.setTooltip("Next voice in the catalog");
    voicePreviousButton.setRepeatSpeed(400, 90, 20);
    voiceNextButton.setRepeatSpeed(400, 90, 20);
    voicePreviousButton.onClick = [this] { stepCatalogVoice(-1); };
    voiceNextButton.onClick = [this] { stepCatalogVoice(1); };
    voicePage.addAndMakeVisible(knobGroupPanel);  // behind everything else
    voicePage.addAndMakeVisible(lcdPanel);
    voicePage.addAndMakeVisible(voicePreviousButton);
    voicePage.addAndMakeVisible(voiceNameButton);
    voicePage.addAndMakeVisible(voiceNextButton);


    voicePage.setLookAndFeel(&hardwareLAF);

    for (std::size_t i = 0; i < parameterIds.size(); ++i)
    {
        const auto parameterName = owner.parameters.getParameter(parameterIds[i])->getName(64);
        auto& label = parameterLabels[i];
        label.setText(parameterName, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xFF2A2820));
        label.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        label.setInterceptsMouseClicks(false, false);
        voicePage.addAndMakeVisible(label);

        if (i < steppers.size())
        {
            auto& stepper = steppers[i];
            voicePage.addAndMakeVisible(stepper);
            attachments[i] = std::make_unique<SliderAttachment>(owner.parameters,
                                                                parameterIds[i],
                                                                stepper.attachmentSlider());
            stepper.syncDisplayedValue();
            continue;
        }

        auto& slider = knobSliders[i - steppers.size()];
        slider.setLookAndFeel(&hardwareLAF);
        slider.setName(parameterName);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 22);
        slider.setDoubleClickReturnValue(true,
                                         owner.parameters.getParameterRange(parameterIds[i])
                                             .convertFrom0to1(
                                                 owner.parameters.getParameter(parameterIds[i])
                                                     ->getDefaultValue()));
        voicePage.addAndMakeVisible(slider);
        attachments[i] = std::make_unique<SliderAttachment>(owner.parameters,
                                                            parameterIds[i],
                                                            slider);
    }

    steppers[1].setDiscreteValues({ 0, 64, 126, 127 });
    steppers[1].setValueChangeCallback([this] { updateVoiceChoices(true, true); });
    steppers[2].setValueChangeCallback([this] { updateVoiceChoices(false, true); });
    steppers[3].setValueChangeCallback([this] { updateVoiceName(); });
    updateVoiceChoices(false, false);

    xgButton.setTooltip("Commanded MIDI mode; hardware confirmation is not available yet");
    xgButton.onClick = [this]
    {
        owner.setXgModeEnabled(!owner.isXgModeEnabled());
        refreshXgButton();
    };
    refreshXgButton();
    resetButton.setTooltip("Reset sound, controller, envelope and effect edits; keep Part and voice");
    resetButton.onClick = [this] { owner.resetEditingParameters(); };
    settingsButton.setTooltip("Open Settings");
    settingsButton.onClick = [this] {
        if (settingsOverlay != nullptr)
            settingsOverlay->setVisible(!settingsOverlay->isVisible());
    };
    addAndMakeVisible(xgButton);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(settingsButton);

    settingsOverlay = std::make_unique<SettingsOverlay>(owner);
    addChildComponent(*settingsOverlay);
    settingsOverlay->setAlwaysOnTop(true);

    const auto makeIds = [](std::initializer_list<const char*> values)
    {
        juce::StringArray result;
        for (const auto* value : values)
            result.add(value);
        return result;
    };

    partPage = std::make_unique<ParameterPage>(
        owner.parameters,
        makeIds({ "monoPoly", "keyAssign", "partMode", "noteShift", "detune",
                  "velocityDepth", "velocityOffset", "noteLimitLow", "noteLimitHigh",
                  "dryLevel", "decay", "portamentoSwitch", "portamentoTime" }),
        "Per-part performance, keyboard range, dynamics and portamento.",
        &hardwareLAF);
    controllerPage = std::make_unique<ParameterPage>(
        owner.parameters,
        makeIds({ "vibratoRate", "vibratoDepth", "vibratoDelay",
                  "mwPitch", "mwFilter", "mwAmplitude", "mwLfoPitch", "mwLfoFilter",
                  "mwLfoAmplitude", "bendPitch", "bendFilter", "bendAmplitude",
                  "bendLfoPitch", "bendLfoFilter", "bendLfoAmplitude",
                  "aftertouchPitch", "aftertouchFilter", "aftertouchAmplitude",
                  "aftertouchLfoPitch", "aftertouchLfoFilter", "aftertouchLfoAmplitude" }),
        "Mod Wheel, Pitch Bend and Channel Aftertouch response for the selected part.",
        &hardwareLAF);
    pitchPage = std::make_unique<ParameterPage>(
        owner.parameters,
        makeIds({ "pitchEgInitial", "pitchEgAttack", "pitchEgReleaseLevel",
                  "pitchEgReleaseTime" }),
        "Pitch envelope values are relative to the selected voice (-64...+63).",
        &hardwareLAF);

    juce::StringArray reverbIds { "reverbType", "reverbReturn", "reverbPan" };
    juce::StringArray chorusIds { "chorusType", "chorusReturn", "chorusPan",
                                  "chorusToReverb" };
    juce::StringArray variationIds { "variationType", "variationReturn", "variationPan",
                                     "variationToReverb", "variationToChorus",
                                     "variationConnection", "variationPart",
                                     "mwVariationDepth", "pbVariationDepth",
                                     "atVariationDepth", "ac1VariationDepth",
                                     "ac2VariationDepth" };
    for (int number = 1; number <= 16; ++number)
    {
        reverbIds.add("reverbParam" + juce::String(number));
        chorusIds.add("chorusParam" + juce::String(number));
        variationIds.add("variationParam" + juce::String(number));
    }
    reverbPage = std::make_unique<ParameterPage>(
        owner.parameters, reverbIds,
        "Effect parameters 1-16 follow the selected Yamaha algorithm; values are shown raw (0-127).",
        &hardwareLAF);
    chorusPage = std::make_unique<ParameterPage>(
        owner.parameters, chorusIds,
        "Effect parameters 1-16 follow the selected Yamaha algorithm; values are shown raw (0-127).",
        &hardwareLAF);
    variationPage = std::make_unique<ParameterPage>(
        owner.parameters, variationIds,
        "Variation includes delays and insertion effects. Params 1-10 are raw 14-bit values; 11-16 are 7-bit.",
        &hardwareLAF);
    const auto tabColour = juce::Colour(HardwareLookAndFeel::panelBg);
    tabs.setTabBarDepth(32);
    tabs.addTab("Voice",            tabColour, &voicePage,            false);
    tabs.addTab("Part",             tabColour, partPage.get(),        false);
    tabs.addTab("Controllers",      tabColour, controllerPage.get(),  false);
    tabs.addTab("Pitch EG",         tabColour, pitchPage.get(),       false);
    tabs.addTab("Reverb",           tabColour, reverbPage.get(),      false);
    tabs.addTab("Chorus",           tabColour, chorusPage.get(),      false);
    tabs.addTab("Variation / Delay",tabColour, variationPage.get(),   false);
    addAndMakeVisible(tabs);

    setResizable(true, true);
    setResizeLimits(900, 650, 1500, 1050);
    setSize(1120, 780);
    updateEffectLabels();
    startTimerHz(4);

    // Register listener to detect part changes for per-part state recall
    owner.parameters.addParameterListener("part", this);
    // Seed lastKnownPart from current value
    if (const auto* raw = owner.parameters.getRawParameterValue("part"))
        lastKnownPart = juce::roundToInt(raw->load());

    // Initialize perPartState for all 16 parts with initial parameter defaults
    for (int p = 1; p <= 16; ++p)
    {
        auto& slot = perPartState[static_cast<std::size_t>(p - 1)];
        for (const auto& id : partSaveIds())
            if (auto* param = owner.parameters.getParameter(id))
                slot[id] = param->getValue();
    }
}

QY70ControllerAudioProcessorEditor::~QY70ControllerAudioProcessorEditor()
{
    owner.parameters.removeParameterListener("part", this);
    setLookAndFeel (nullptr);
    stopTimer();
}

void QY70ControllerAudioProcessorEditor::timerCallback()
{
    refreshXgButton();
    updateEffectLabels();
}

// ── Per-part state recall ────────────────────────────────────────────────────

const juce::StringArray& QY70ControllerAudioProcessorEditor::partSaveIds()
{
    // All parameters that are meaningfully per-part:
    // voice selection + all mix/performance parameters
    static const juce::StringArray ids {
        "bankMsb", "bankLsb", "program",
        "volume", "pan", "cutoff", "resonance", "attack", "release",
        "chorus", "reverb", "variation",
        "monoPoly", "keyAssign", "partMode", "noteShift", "detune",
        "velocityDepth", "velocityOffset", "noteLimitLow", "noteLimitHigh",
        "dryLevel", "decay", "portamentoSwitch", "portamentoTime",
        "vibratoRate", "vibratoDepth", "vibratoDelay",
        "mwPitch", "mwFilter", "mwAmplitude",
        "bendPitch", "bendFilter", "bendAmplitude",
        "aftertouchPitch", "aftertouchFilter", "aftertouchAmplitude",
        "pitchEgInitial", "pitchEgAttack", "pitchEgReleaseLevel", "pitchEgReleaseTime"
    };
    return ids;
}

void QY70ControllerAudioProcessorEditor::saveCurrentPartState()
{
    if (lastKnownPart < 1 || lastKnownPart > 16) return;
    auto& slot = perPartState[static_cast<std::size_t>(lastKnownPart - 1)];
    for (const auto& id : partSaveIds())
        if (auto* param = owner.parameters.getParameter(id))
            slot[id] = param->getValue();  // normalised 0..1
}

void QY70ControllerAudioProcessorEditor::restorePartState(int part)
{
    if (part < 1 || part > 16) return;
    const auto& slot = perPartState[static_cast<std::size_t>(part - 1)];
    if (slot.empty()) return;  // no saved state for this part yet

    {
        const juce::ScopedValueSetter<bool> guard(updatingVoiceChoices, true);
        for (const auto& [id, value] : slot)
        {
            if (auto* param = owner.parameters.getParameter(id))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(value);
                param->endChangeGesture();
            }
        }
    }

    // Refresh voice display after restoring
    updateVoiceChoices(false, false);
}

void QY70ControllerAudioProcessorEditor::parameterChanged(const juce::String& parameterID,
                                                          float newValue)
{
    if (parameterID != "part") return;
    const int newPart = juce::roundToInt(newValue);
    if (newPart == lastKnownPart) return;

    const int oldPart = lastKnownPart;
    lastKnownPart = newPart;

    juce::MessageManager::callAsync([this, oldPart, newPart]
    {
        if (oldPart >= 1 && oldPart <= 16)
        {
            auto& slot = perPartState[static_cast<std::size_t>(oldPart - 1)];
            for (const auto& id : partSaveIds())
                if (auto* param = owner.parameters.getParameter(id))
                    slot[id] = param->getValue();
        }
        restorePartState(newPart);
    });
}

void QY70ControllerAudioProcessorEditor::updateEffectLabels()
{
    const auto update = [this](ParameterPage* page,
                               qy70::EffectBlock block,
                               const char* typeId,
                               const char* parameterPrefix)
    {
        if (page == nullptr)
            return;
        const auto* raw = owner.parameters.getRawParameterValue(typeId);
        const auto typeIndex = raw != nullptr ? juce::roundToInt(raw->load()) : 0;
        const auto names = qy70::effectParameterNames(block, typeIndex);
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            juce::String label;
            if (names[index].empty())
                label = typeIndex == 0 ? "Unused"
                                       : "Parameter " + juce::String(static_cast<int>(index) + 1)
                                             + " (raw)";
            else
                label = juce::String::fromUTF8(names[index].data(),
                                               static_cast<int>(names[index].size()));
            page->setParameterLabel(juce::String(parameterPrefix)
                                        + juce::String(static_cast<int>(index) + 1),
                                    label);
        }
    };

    update(reverbPage.get(), qy70::EffectBlock::reverb, "reverbType", "reverbParam");
    update(chorusPage.get(), qy70::EffectBlock::chorus, "chorusType", "chorusParam");
    update(variationPage.get(), qy70::EffectBlock::variation,
           "variationType", "variationParam");
}

void QY70ControllerAudioProcessorEditor::refreshXgButton()
{
    const auto enabled = owner.isXgModeEnabled();
    xgButton.setButtonText(enabled ? "XG ON" : "XG OFF");
    xgButton.setColour(juce::TextButton::buttonColourId,
                       enabled ? juce::Colour(HardwareLookAndFeel::btnXgOn)
                               : juce::Colour(HardwareLookAndFeel::btnXgOff));
    xgButton.setColour(juce::TextButton::textColourOffId,
                       juce::Colour(HardwareLookAndFeel::panelLight));
}

void QY70ControllerAudioProcessorEditor::updateVoiceChoices(bool bankChanged, bool resetLsb)
{
    if (updatingVoiceChoices)
        return;

    const juce::ScopedValueSetter<bool> guard(updatingVoiceChoices, true);
    const auto bankMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto programs = qy70::validProgramsForBank(bankMsb);
    steppers[2].setDiscreteValues(std::vector<double>(programs.begin(), programs.end()));

    if (bankChanged && !programs.empty())
        steppers[2].attachmentSlider().setValue(programs.front(), juce::sendNotificationSync);

    const auto program = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto lsbValues = qy70::validLsbValues(bankMsb, program);
    steppers[3].setDiscreteValues(std::vector<double>(lsbValues.begin(), lsbValues.end()));
    if (resetLsb)
        steppers[3].attachmentSlider().setValue(0, juce::sendNotificationSync);

    updateVoiceName();
}

void QY70ControllerAudioProcessorEditor::updateVoiceName()
{
    const auto bankMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto program = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto bankLsb = juce::roundToInt(steppers[3].attachmentSlider().getValue());
    const auto mode = qy70::voiceModeName(bankMsb);
    const auto voice = qy70::voiceName(bankMsb, bankLsb, program);
    const auto modeText = juce::String::fromUTF8(mode.data(), static_cast<int>(mode.size()));
    const auto voiceText = juce::String::fromUTF8(voice.data(), static_cast<int>(voice.size()));

    voiceNameButton.setButtonText(modeText + "  ·  " + voiceText
                                  + "   (Patch " + juce::String(program)
                                  + ", Variation " + juce::String(bankLsb) + ")"
                                  + juce::String::fromUTF8("\xe2\x96\xbc"));
    // Ensure the dropdown arrow (▼) is right-aligned via button text right-pad — handled in drawButtonText
}

void QY70ControllerAudioProcessorEditor::showVoiceMenu()
{
    auto catalog = qy70::voiceCatalog();
    struct CategoryMenu
    {
        juce::String name;
        juce::PopupMenu menu;
    };
    std::vector<CategoryMenu> categories;

    const auto currentMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto currentProgram = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto currentLsb = juce::roundToInt(steppers[3].attachmentSlider().getValue());

    for (std::size_t index = 0; index < catalog.size(); ++index)
    {
        const auto& voice = catalog[index];
        const auto categoryName = juce::String::fromUTF8(voice.category.data(),
                                                         static_cast<int>(voice.category.size()));
        auto category = std::find_if(categories.begin(), categories.end(),
                                     [&categoryName](const CategoryMenu& item)
                                     {
                                         return item.name == categoryName;
                                     });
        if (category == categories.end())
        {
            categories.push_back({ categoryName, {} });
            category = std::prev(categories.end());
        }

        const auto name = juce::String::fromUTF8(voice.name.data(),
                                                 static_cast<int>(voice.name.size()));
        auto itemText = name + "   P" + juce::String(voice.program);
        if (voice.bankLsb != 0)
            itemText += " · V" + juce::String(voice.bankLsb);

        const auto isCurrent = voice.bankMsb == currentMsb
                               && voice.program == currentProgram
                               && voice.bankLsb == currentLsb;
        category->menu.addItem(static_cast<int>(index) + 1, itemText, true, isCurrent);
    }

    juce::PopupMenu menu;
    for (const auto& category : categories)
        menu.addSubMenu(category.name, category.menu);

    const juce::Component::SafePointer<QY70ControllerAudioProcessorEditor> safeThis(this);

    // Apply LCD color scheme to the popup menu
    menu.setLookAndFeel(&hardwareLAF);

    // Open directly under the voiceNameButton field
    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetComponent(&voiceNameButton)
                           .withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::downwards),
                       [safeThis, catalogCopy = std::move(catalog)](int result)
                       {
                           if (safeThis != nullptr && result > 0
                               && result <= static_cast<int>(catalogCopy.size()))
                               safeThis->selectVoice(catalogCopy[static_cast<std::size_t>(result - 1)]);
                       });
}

void QY70ControllerAudioProcessorEditor::selectVoice(const qy70::VoiceDescriptor& voice)
{
    const juce::ScopedValueSetter<bool> guard(updatingVoiceChoices, true);
    steppers[1].attachmentSlider().setValue(voice.bankMsb, juce::sendNotificationSync);

    const auto programs = qy70::validProgramsForBank(voice.bankMsb);
    steppers[2].setDiscreteValues(std::vector<double>(programs.begin(), programs.end()));
    steppers[2].attachmentSlider().setValue(voice.program, juce::sendNotificationSync);

    const auto variations = qy70::validLsbValues(voice.bankMsb, voice.program);
    steppers[3].setDiscreteValues(std::vector<double>(variations.begin(), variations.end()));
    steppers[3].attachmentSlider().setValue(voice.bankLsb, juce::sendNotificationSync);
    updateVoiceName();
}

void QY70ControllerAudioProcessorEditor::stepCatalogVoice(int direction)
{
    const auto catalog = qy70::voiceCatalog();
    if (catalog.empty())
        return;

    const auto currentMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto currentProgram = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto currentLsb = juce::roundToInt(steppers[3].attachmentSlider().getValue());
    const auto current = std::find_if(catalog.begin(), catalog.end(),
                                      [=](const qy70::VoiceDescriptor& voice)
                                      {
                                          return voice.bankMsb == currentMsb
                                                 && voice.program == currentProgram
                                                 && voice.bankLsb == currentLsb;
                                      });
    const auto currentIndex = current != catalog.end()
                                  ? static_cast<int>(std::distance(catalog.begin(), current))
                                  : 0;
    const auto count = static_cast<int>(catalog.size());
    const auto nextIndex = (currentIndex + (direction < 0 ? -1 : 1) + count) % count;
    selectVoice(catalog[static_cast<std::size_t>(nextIndex)]);
}

void QY70ControllerAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // ── Outer shell: very dark metallic frame ─────────────────────────────
    g.setColour(juce::Colour(0xFF1A1C1E));
    g.fillRoundedRectangle(bounds, 12.0f);

    const auto panelBounds = bounds.reduced(5.0f);

    // ── Multi-stop metallic silver gradient (top → bottom) ────────────────
    // Mimics the QY70's brushed-aluminum surface: bright specular at top,
    // main silver in the middle, subtle shadow at the bottom edge.
    juce::ColourGradient panelGrad(
        juce::Colour(0xFFE6E8E4), panelBounds.getX(), panelBounds.getY(),       // bright specular top
        juce::Colour(0xFF6E7070), panelBounds.getX(), panelBounds.getBottom(),  // deep shadow bottom
        false);
    panelGrad.addColour(0.08,  juce::Colour(HardwareLookAndFeel::panelLight));   // quick drop to normal silver
    panelGrad.addColour(0.38,  juce::Colour(HardwareLookAndFeel::panelBg));      // mid base
    panelGrad.addColour(0.72,  juce::Colour(HardwareLookAndFeel::panelDark));    // lower shadow zone
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(panelBounds, 9.0f);

    // ── Diagonal glint: top-left bright sheen ────────────────────────────
    juce::ColourGradient glint(
        juce::Colour(0x1AFFFFFF), panelBounds.getX(), panelBounds.getY(),
        juce::Colour(0x00FFFFFF),
        panelBounds.getX() + panelBounds.getWidth()  * 0.45f,
        panelBounds.getY() + panelBounds.getHeight() * 0.35f,
        false);
    g.setGradientFill(glint);
    g.fillRoundedRectangle(panelBounds, 9.0f);

    // ── Outer bevel (raised panel look) ──────────────────────────────────
    HardwareLookAndFeel::drawBevel(g, panelBounds, 1.8f, true);

    // ── Fine horizontal texture lines (brushed-metal feel) ───────────────
    g.setColour(juce::Colour(0x08000000));
    for (int row = (int)panelBounds.getY(); row < (int)panelBounds.getBottom(); row += 2)
        g.fillRect((int)panelBounds.getX(), row, (int)panelBounds.getWidth(), 1);
}

void QY70ControllerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);
    // Taller header: 52px, bigger logo
    auto header = area.removeFromTop(52);
    logoComponent.setBounds(header.removeFromLeft(380));
    settingsButton.setBounds(header.removeFromRight(90).reduced(4, 8));
    xgButton.setBounds(header.removeFromRight(90).reduced(4, 8));
    resetButton.setBounds(header.removeFromRight(110).reduced(4, 8));

    area.removeFromTop(6);
    tabs.setBounds(area);
    layoutVoicePage();

    if (settingsOverlay != nullptr)
        settingsOverlay->setBounds(getLocalBounds());
}

void QY70ControllerAudioProcessorEditor::layoutVoicePage()
{
    auto area = voicePage.getLocalBounds().reduced(16);

    // ── LCD labels (on beige) + value row (green only under valueLabel) ─────
    constexpr int labelRowH   = 16;  // compact label row
    constexpr int stepperRowH = 36;  // value display row only
    constexpr int lcdGap      = 5;
    auto labelRow   = area.removeFromTop(labelRowH);
    auto stepperRow = area.removeFromTop(stepperRowH);

    // Hide lcdPanel so no green bleeds under arrow buttons.
    // The valueLabel in each stepper already has its own green background.
    lcdPanel.setBounds ({});

    const auto selectorWidth = labelRow.getWidth() / 4;
    for (std::size_t i = 0; i < 4; ++i)
    {
        auto lCell = labelRow
                         .withX(labelRow.getX() + static_cast<int>(i) * selectorWidth)
                         .withWidth(selectorWidth)
                         .reduced(8, 0);
        auto sCell = stepperRow
                         .withX(stepperRow.getX() + static_cast<int>(i) * selectorWidth)
                         .withWidth(selectorWidth)
                         .reduced(8, 3);
        parameterLabels[i].setColour(juce::Label::textColourId, juce::Colour(0xFF2A2820));
        parameterLabels[i].setFont(juce::FontOptions(9.5f, juce::Font::bold));
        parameterLabels[i].setBounds(lCell);
        steppers[i].setBounds(sCell);
    }

    // ── Voice name selector ──────────────────────────────────────────────
    area.removeFromTop(lcdGap);
    auto voiceRow = area.removeFromTop(32).reduced(12, 2);
    voicePreviousButton.setBounds(voiceRow.removeFromLeft(36));
    voiceNextButton.setBounds(voiceRow.removeFromRight(36));
    voiceNameButton.setBounds(voiceRow.reduced(3, 0));
    area.removeFromTop(5);

    // ── 3×3 knob grid: centered vertically in remaining space ─────────────
    constexpr int columns    = 3;
    constexpr int cellHeight = 126;
    constexpr int totalGridH = 3 * cellHeight;
    const auto cellWidth = area.getWidth() / columns;

    // Centre the 3-row grid inside whatever space remains
    const int vPad = juce::jmax (0, (area.getHeight() - totalGridH) / 2);
    const int gridTop = area.getY() + vPad;

    // Position sunken panel: extend to fill full voicePage area (matching other tabs)
    knobGroupPanel.setBounds (voicePage.getLocalBounds().reduced (4));


    for (std::size_t i = 4; i < parameterIds.size(); ++i)
    {
        const auto cellIndex = static_cast<int>(i - 4);
        auto cell = juce::Rectangle<int>(
                        area.getX() + (cellIndex % columns) * cellWidth,
                        gridTop     + (cellIndex / columns) * cellHeight,
                        cellWidth,
                        cellHeight)
                        .reduced(10, 4);
        parameterLabels[i].setBounds(cell.removeFromTop(25));
        knobSliders[i - steppers.size()].setBounds(cell.reduced(6, 0));
    }
}



