using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Data.Core;
using Avalonia.Data.Core.Plugins;
using Avalonia.Markup.Xaml;
using Snap.Hutao.Remastered.Deployment.ViewModels;
using Snap.Hutao.Remastered.Deployment.Views;
using System.Linq;
using System.Threading.Tasks;

namespace Snap.Hutao.Remastered.Deployment
{
    public partial class App : Application
    {
        public override void Initialize()
        {
            AvaloniaXamlLoader.Load(this);
        }

        public override void OnFrameworkInitializationCompleted()
        {
            if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
            {
                var viewModel = new MainWindowViewModel();
                desktop.MainWindow = new MainWindow
                {
                    DataContext = viewModel,
                };

                if (Program.StartupArgs != null && Program.StartupArgs.Length > 0 && Program.StartupArgs[0].ToLowerInvariant() == "update")
                {
                    Task.Delay(1000).ContinueWith(_ =>
                    {
                        Avalonia.Threading.Dispatcher.UIThread.Post(() =>
                        {
                            viewModel.StartAutoUpdate();
                        });
                    });
                }
            }

            base.OnFrameworkInitializationCompleted();
        }
    }
}
