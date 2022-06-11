#include "pch.h"

using namespace winrt;

using namespace Windows;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Composition;

#include "trrojan/executive.h"
#include "trrojan/d3d11/plugin.h"
#include "trrojan/d3d11/utilities.h"

struct App : implements<App, IFrameworkViewSource, IFrameworkView>
{
    CompositionTarget m_target{ nullptr };
    VisualCollection m_visuals{ nullptr };
    Visual m_selected{ nullptr };
    float2 m_offset{};

    IFrameworkView CreateView()
    {
        return *this;
    }

    void Initialize(CoreApplicationView const &)
    {
    }

    void Load(hstring const&)
    {
    }

    void Uninitialize()
    {
    }

    void Run()
    {
        CoreWindow window = CoreWindow::GetForCurrentThread();
        window.Activate();

        //get AppData folder with write access
        winrt::Windows::Storage::StorageFolder localFolder{ winrt::Windows::Storage::ApplicationData::Current().LocalFolder() };//for local saving for future
        //winrt::Windows::Storage::StorageFolder roamingFolder{ winrt::Windows::Storage::ApplicationData::Current().RoamingFolder() };

        winrt::hstring path = localFolder.Path();

        //convert folder name from wchar to ascii
        std::wstring folderNameW(path);
        std::string folderNameA(folderNameW.begin(), folderNameW.end());

        /* Configure the executive. */
        trrojan::cmd_line cmdLine;
        cmdLine.push_back("--output");
        cmdLine.push_back(folderNameA +"\\test.csv");
        //cmdLine.push_back("--log");
        //cmdLine.push_back(folderNameA + "\\log.txt");
        auto output = trrojan::open_output(cmdLine);
        trrojan::cool_down cool_down;
        trrojan::executive exe;

        trrojan::plugin plugin = std::make_shared<trrojan::d3d11::plugin>(window);
        exe.add_plugin(plugin, cmdLine);

        auto trroll_path = trrojan::GetAppFolder().string() + "Assets/demo.trroll";

        exe.trroll(trroll_path, *output, cool_down);

        // Copy from local folder to usb pen drive?
        auto csv_output{ localFolder.GetFileAsync(L"test.csv").get()};
        winrt::Windows::Storage::StorageFolder removableFolder{ winrt::Windows::Storage::KnownFolders::RemovableDevices() };
        auto folders{ removableFolder.GetFoldersAsync().get()};
        //Windows::Foundation::Collections::IVectorView<Windows::Storage::StorageFolder> itemsInFolder{
        // };
        auto folder_cnt = folders.Size();

        if (folder_cnt > 0) {
            auto folder = folders.GetAt(0);
            auto copy_csv_output = csv_output.CopyAsync(folder);
        }

        CoreDispatcher dispatcher = window.Dispatcher();
        dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
    }

    void SetWindow(CoreWindow const & window)
    {
        //Compositor compositor;
        //ContainerVisual root = compositor.CreateContainerVisual();
        //m_target = compositor.CreateTargetForCurrentView();
        //m_target.Root(root);
        //m_visuals = root.Children();
        //
        //window.PointerPressed({ this, &App::OnPointerPressed });
        //window.PointerMoved({ this, &App::OnPointerMoved });
        //
        //window.PointerReleased([&](auto && ...)
        //{
        //    m_selected = nullptr;
        //});
    }

    void OnPointerPressed(IInspectable const &, PointerEventArgs const & args)
    {
        float2 const point = args.CurrentPoint().Position();

        for (Visual visual : m_visuals)
        {
            float3 const offset = visual.Offset();
            float2 const size = visual.Size();

            if (point.x >= offset.x &&
                point.x < offset.x + size.x &&
                point.y >= offset.y &&
                point.y < offset.y + size.y)
            {
                m_selected = visual;
                m_offset.x = offset.x - point.x;
                m_offset.y = offset.y - point.y;
            }
        }

        if (m_selected)
        {
            m_visuals.Remove(m_selected);
            m_visuals.InsertAtTop(m_selected);
        }
        else
        {
            AddVisual(point);
        }
    }

    void OnPointerMoved(IInspectable const &, PointerEventArgs const & args)
    {
        if (m_selected)
        {
            float2 const point = args.CurrentPoint().Position();

            m_selected.Offset(
            {
                point.x + m_offset.x,
                point.y + m_offset.y,
                0.0f
            });
        }
    }

    void AddVisual(float2 const point)
    {
        Compositor compositor = m_visuals.Compositor();
        SpriteVisual visual = compositor.CreateSpriteVisual();

        static Color colors[] =
        {
            { 0xDC, 0x5B, 0x9B, 0xD5 },
            { 0xDC, 0xED, 0x7D, 0x31 },
            { 0xDC, 0x70, 0xAD, 0x47 },
            { 0xDC, 0xFF, 0xC0, 0x00 }
        };

        static unsigned last = 0;
        unsigned const next = ++last % _countof(colors);
        visual.Brush(compositor.CreateColorBrush(colors[next]));

        float const BlockSize = 100.0f;

        visual.Size(
        {
            BlockSize,
            BlockSize
        });

        visual.Offset(
        {
            point.x - BlockSize / 2.0f,
            point.y - BlockSize / 2.0f,
            0.0f,
        });

        m_visuals.InsertAtTop(visual);

        m_selected = visual;
        m_offset.x = -BlockSize / 2.0f;
        m_offset.y = -BlockSize / 2.0f;
    }
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    CoreApplication::Run(make<App>());
}