#include "application.h"
#include "imgui.h"
#include "text.h"

namespace streamdiscord
{
namespace
{
void hint(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

void heading(const char* number, const char* title)
{
    ImGui::TextColored({0.68f, 0.56f, 1, 1}, "%s", number);
    ImGui::SameLine();
    ImGui::TextUnformatted(title);
}
} // namespace

void Application::drawApplicationPicker(const char* id, char* buffer, size_t size)
{
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo(id, "Sélectionner une fenêtre ouverte"))
    {
        if (choices.empty())
        {
            ImGui::TextDisabled("Aucune fenêtre disponible");
        }
        for (size_t i = 0; i < choices.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            auto label = toUtf8(choices[i].exe + L" — " + choices[i].title);
            if (ImGui::Selectable(label.c_str()))
            {
                copyUtf8(buffer, size, choices[i].exe);
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
}

void Application::drawHeader()
{
    float scale = interfaceRenderer.dpiScale();
    ImGui::TextColored({0.70f, 0.60f, 1, 1}, "S T R E A M D I S C O R D");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 105 * scale);
    ImGui::TextDisabled("VERSION 0.2");
    ImGui::SetWindowFontScale(1.65f);
    ImGui::TextUnformatted("Une image. Le son au choix.");
    ImGui::SetWindowFontScale(1);
    hint("Sources indépendantes, capture automatique et fenêtre de diffusion dédiée.");
    ImGui::Spacing();
}

void Application::drawVideoCard(float width, float scale)
{
    ImGui::BeginChild("video-card", {width, 278 * scale},
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    heading("01", "VIDÉO");
    hint("Application à capturer");
    ImGui::BeginDisabled(armed);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##video", "Nom de l'exécutable", videoInput, sizeof(videoInput));
    drawApplicationPicker("##video-window", videoInput, sizeof(videoInput));
    if (ImGui::Button("Profil TFT"))
    {
        copyUtf8(videoInput, sizeof(videoInput), L"League of Legends.exe");
    }
    ImGui::EndDisabled();
    ImGui::Spacing();
    hint("Détection automatique au lancement de l'application.");
    ImGui::EndChild();
}

void Application::drawAudioCard(float width, float scale)
{
    ImGui::BeginChild("audio-card", {width, 278 * scale},
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    heading("02", "AUDIO");
    hint("Application à écouter");
    ImGui::BeginDisabled(armed);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##audio", "Identique à la vidéo", audioInput, sizeof(audioInput));
    drawApplicationPicker("##audio-window", audioInput, sizeof(audioInput));
    if (ImGui::Button("Utiliser l'audio de la vidéo"))
    {
        audioInput[0] = '\0';
    }
    ImGui::EndDisabled();
    ImGui::Spacing();
    hint("Sans source distincte, l'audio suit la source vidéo.");
    ImGui::EndChild();
}

void Application::drawOutputCard(float scale)
{
    ImGui::BeginChild("output-card", {0, 164 * scale},
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    heading("03", "SORTIE AUDIO");
    std::string preview = "Relais désactivé";
    bool defaultOutput = false;
    for (auto const& output : outputs)
    {
        if (output.id == outputId)
        {
            preview = toUtf8(output.name);
            defaultOutput = output.isDefault;
        }
    }
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 132 * scale);
    if (ImGui::BeginCombo("##output", preview.c_str()))
    {
        if (ImGui::Selectable("Relais désactivé", outputId.empty()))
        {
            outputId.clear();
            save();
            if (armed)
            {
                reconcile();
            }
        }
        for (auto const& output : outputs)
        {
            auto label = toUtf8(output.name) + (output.isDefault ? " · Par défaut" : "");
            if (ImGui::Selectable(label.c_str(), output.id == outputId))
            {
                outputId = output.id;
                save();
                if (armed)
                {
                    reconcile();
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Actualiser", {120 * scale, 0}))
    {
        choices = enumerateWindows();
        refreshOutputs();
        save();
        if (armed)
        {
            reconcile();
        }
    }
    if (outputId.empty())
    {
        hint("Aucune sortie sélectionnée : le stream ne transmet pas d'audio.");
    }
    else if (defaultOutput)
    {
        ImGui::TextColored({1, 0.72f, 0.40f, 1},
                           "Sortie par défaut : un doublon audio local est possible.");
    }
    else
    {
        hint("Une sortie non écoutée permet d'éviter le doublon audio local.");
    }
    ImGui::EndChild();
}

void Application::drawStatusCard(float scale)
{
    ImGui::BeginChild("status-card", {0, 92 * scale},
                      ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImVec4 stateColor = !armed     ? ImVec4{0.58f, 0.63f, 0.73f, 1}
                        : captured ? ImVec4{0.38f, 0.85f, 0.66f, 1}
                                   : ImVec4{1, 0.73f, 0.40f, 1};
    ImGui::TextColored(stateColor, "%s",
                       !armed     ? "À L'ARRÊT"
                       : captured ? "CAPTURE ACTIVE"
                                  : "EN ATTENTE");
    auto status = toUtf8(statusText);
    ImGui::TextWrapped("%s", status.c_str());
    ImGui::EndChild();
}

void Application::drawActions(float scale)
{
    if (!uiError.empty())
    {
        ImGui::TextColored({1, 0.5f, 0.5f, 1}, "%s", uiError.c_str());
    }
    ImGui::PushStyleColor(ImGuiCol_Button,
                          armed ? ImVec4{0.42f, 0.19f, 0.26f, 1} : ImVec4{0.42f, 0.30f, 0.76f, 1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          armed ? ImVec4{0.55f, 0.23f, 0.32f, 1} : ImVec4{0.52f, 0.39f, 0.88f, 1});
    if (ImGui::Button(armed ? "Arrêter la capture" : "Démarrer la capture",
                      {240 * scale, 46 * scale}))
    {
        uiError.clear();
        if (armed)
        {
            armed = false;
            stop();
            statusText = L"Capture arrêtée";
        }
        else
        {
            startCapture();
        }
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    if (ImGui::Button("Ouvrir la diffusion", {210 * scale, 46 * scale}))
    {
        showDiffusion();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(armed);
    if (ImGui::Button("Actualiser les sources", {210 * scale, 46 * scale}))
    {
        choices = enumerateWindows();
    }
    ImGui::EndDisabled();
    ImGui::Spacing();
    hint("Discord  /  Partage d'application : StreamDiscord — Diffusion, avec le son activé.");
}

void Application::drawInterface()
{
    const auto& io = ImGui::GetIO();
    const float scale = interfaceRenderer.dpiScale();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("StreamDiscord", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    drawHeader();
    const float cardWidth =
        (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20 * scale, 18 * scale});
    drawVideoCard(cardWidth, scale);
    ImGui::SameLine();
    drawAudioCard(cardWidth, scale);
    drawOutputCard(scale);
    drawStatusCard(scale);
    ImGui::PopStyleVar();
    drawActions(scale);
    ImGui::End();
}
} // namespace streamdiscord
