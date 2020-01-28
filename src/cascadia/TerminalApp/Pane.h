// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
// Module Name:
// - Pane.h
//
// Abstract:
// - Panes are an abstraction by which the terminal can dislay multiple terminal
//   instances simultaneously in a single terminal window. While tabs allow for
//   a single terminal window to have many terminal sessions running
//   simultaneously within a single window, only one tab can be visible at a
//   time. Panes, on the other hand, allow a user to have many different
//   terminal sessions visible to the user within the context of a single window
//   at the same time. This can enable greater productivity from the user, as
//   they can see the output of one terminal window while working in another.
// - See doc/cascadia/Panes.md for a detailed description.
//
// Author:
// - Mike Griese (zadjii-msft) 16-May-2019

#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/TerminalApp.h>
#include "../../cascadia/inc/cppwinrt_utils.h"

enum class Borders : int
{
    None = 0x0,
    Top = 0x1,
    Bottom = 0x2,
    Left = 0x4,
    Right = 0x8
};
DEFINE_ENUM_FLAG_OPERATORS(Borders);

class Pane : public std::enable_shared_from_this<Pane>
{
public:
    Pane(const GUID& profile,
         const winrt::Microsoft::Terminal::TerminalControl::TermControl& control,
         const bool lastFocused = false);

    std::shared_ptr<Pane> GetActivePane();
    winrt::Microsoft::Terminal::TerminalControl::TermControl GetTerminalControl();
    std::optional<GUID> GetFocusedProfile();

    winrt::Windows::UI::Xaml::Controls::Grid GetRootElement();

    bool WasLastFocused() const noexcept;
    void UpdateVisuals();
    void ClearActive();
    void SetActive();

    void UpdateSettings(const winrt::Microsoft::Terminal::Settings::TerminalSettings& settings,
                        const GUID& profile);
    void ResizeContent(const winrt::Windows::Foundation::Size& newSize);
    void Relayout();
    bool ResizePane(const winrt::Microsoft::Terminal::Settings::Direction& direction);
    bool NavigateFocus(const winrt::Microsoft::Terminal::Settings::Direction& direction);

    bool CanSplit(winrt::Microsoft::Terminal::Settings::SplitState splitType);
    std::pair<std::shared_ptr<Pane>, std::shared_ptr<Pane>> Split(winrt::Microsoft::Terminal::Settings::SplitState splitType,
                                                                  const GUID& profile,
                                                                  const winrt::Microsoft::Terminal::TerminalControl::TermControl& control);
    float CalcSnappedDimension(const bool widthOrHeight, const float dimension) const;

    void Shutdown();
    void Close();

    WINRT_CALLBACK(Closed, winrt::Windows::Foundation::EventHandler<winrt::Windows::Foundation::IInspectable>);
    DECLARE_EVENT(GotFocus, _GotFocusHandlers, winrt::delegate<std::shared_ptr<Pane>>);

private:
    struct SnapSizeResult;
    struct SnapChildrenSizeResult;
    struct LayoutSizeNode;

    winrt::Windows::UI::Xaml::Controls::Grid _root{};
    winrt::Windows::UI::Xaml::Controls::Border _border{};
    winrt::Microsoft::Terminal::TerminalControl::TermControl _control{ nullptr };
    static winrt::Windows::UI::Xaml::Media::SolidColorBrush s_focusedBorderBrush;
    static winrt::Windows::UI::Xaml::Media::SolidColorBrush s_unfocusedBorderBrush;

    std::shared_ptr<Pane> _firstChild{ nullptr };
    std::shared_ptr<Pane> _secondChild{ nullptr };
    winrt::Microsoft::Terminal::Settings::SplitState _splitState{ winrt::Microsoft::Terminal::Settings::SplitState::None };
    float _desiredSplitPosition;

    bool _lastActive{ false };
    std::optional<GUID> _profile{ std::nullopt };
    winrt::event_token _connectionStateChangedToken{ 0 };
    winrt::event_token _firstClosedToken{ 0 };
    winrt::event_token _secondClosedToken{ 0 };

    winrt::Windows::UI::Xaml::UIElement::GotFocus_revoker _gotFocusRevoker;

    std::shared_mutex _createCloseLock{};

    Borders _borders{ Borders::None };

    bool _IsLeaf() const noexcept;
    bool _HasFocusedChild() const noexcept;
    void _SetupChildCloseHandlers();

    bool _CanSplit(winrt::Microsoft::Terminal::Settings::SplitState splitType);
    std::pair<std::shared_ptr<Pane>, std::shared_ptr<Pane>> _Split(winrt::Microsoft::Terminal::Settings::SplitState splitType,
                                                                   const GUID& profile,
                                                                   const winrt::Microsoft::Terminal::TerminalControl::TermControl& control);

    void _CreateRowColDefinitions(const winrt::Windows::Foundation::Size& rootSize);
    void _CreateSplitContent();
    void _ApplySplitDefinitions();
    void _UpdateBorders();

    bool _Resize(const winrt::Microsoft::Terminal::Settings::Direction& direction);
    bool _NavigateFocus(const winrt::Microsoft::Terminal::Settings::Direction& direction);

    void _CloseChild(const bool closeFirst);
    winrt::fire_and_forget _CloseChildRoutine(const bool closeFirst);

    void _FocusFirstChild();
    void _ControlConnectionStateChangedHandler(const winrt::Microsoft::Terminal::TerminalControl::TermControl& sender, const winrt::Windows::Foundation::IInspectable& /*args*/);
    void _ControlGotFocusHandler(winrt::Windows::Foundation::IInspectable const& sender,
                                 winrt::Windows::UI::Xaml::RoutedEventArgs const& e);

    std::pair<float, float> _CalcChildrenSizes(const float fullSize) const;
    SnapChildrenSizeResult _CalcSnappedChildrenSizes(const bool widthOrHeight, const float fullSize) const;
    SnapSizeResult _CalcSnappedDimension(const bool widthOrHeight, const float dimension) const;
    void _AdvanceSnappedDimension(const bool widthOrHeight, LayoutSizeNode& sizeNode) const;

    winrt::Windows::Foundation::Size _GetMinSize() const;
    LayoutSizeNode _CreateMinSizeTree(const bool widthOrHeight) const;
    float _ClampSplitPosition(const bool widthOrHeight, const float requestedValue, const float totalSize) const;

    winrt::Microsoft::Terminal::Settings::SplitState _convertAutomaticSplitState(const winrt::Microsoft::Terminal::Settings::SplitState& splitType) const;
    // Function Description:
    // - Returns true if the given direction can be used with the given split
    //   type.
    // - This is used for pane resizing (which will need a pane separator
    //   that's perpendicular to the direction to be able to move the separator
    //   in that direction).
    // - Additionally, it will be used for moving focus between panes, which
    //   again happens _across_ a separator.
    // Arguments:
    // - direction: The Direction to compare
    // - splitType: The winrt::TerminalApp::SplitState to compare
    // Return Value:
    // - true iff the direction is perpendicular to the splitType. False for
    //   SplitState::None.
    static constexpr bool DirectionMatchesSplit(const winrt::Microsoft::Terminal::Settings::Direction& direction,
                                                const winrt::Microsoft::Terminal::Settings::SplitState& splitType)
    {
        if (splitType == winrt::Microsoft::Terminal::Settings::SplitState::None)
        {
            return false;
        }
        else if (splitType == winrt::Microsoft::Terminal::Settings::SplitState::Horizontal)
        {
            return direction == winrt::Microsoft::Terminal::Settings::Direction::Up ||
                   direction == winrt::Microsoft::Terminal::Settings::Direction::Down;
        }
        else if (splitType == winrt::Microsoft::Terminal::Settings::SplitState::Vertical)
        {
            return direction == winrt::Microsoft::Terminal::Settings::Direction::Left ||
                   direction == winrt::Microsoft::Terminal::Settings::Direction::Right;
        }
        return false;
    }

    static void _SetupResources();

    struct SnapSizeResult
    {
        float lower;
        float higher;
    };

    struct SnapChildrenSizeResult
    {
        std::pair<float, float> lower;
        std::pair<float, float> higher;
    };

    // Helper structure that builds a (roughly) binary tree corresponding
    // to the pane tree. Used for layouting panes with snapped sizes.
    struct LayoutSizeNode
    {
        float size;
        bool isMinimumSize;
        std::unique_ptr<LayoutSizeNode> firstChild;
        std::unique_ptr<LayoutSizeNode> secondChild;

        // These two fields hold next possible snapped values of firstChild and
        // secondChild. Although that could be calculated from these fields themself,
        // it would be wasteful as we have to know these values more often than for
        // simple increment. Hence we cache that here.
        std::unique_ptr<LayoutSizeNode> nextFirstChild;
        std::unique_ptr<LayoutSizeNode> nextSecondChild;

        LayoutSizeNode(const float minSize);
        LayoutSizeNode(const LayoutSizeNode& other);

        LayoutSizeNode& operator=(const LayoutSizeNode& other);

    private:
        void _AssignChildNode(std::unique_ptr<LayoutSizeNode>& nodeField, const LayoutSizeNode* const newNode);
    };
};
