namespace MyPluginUi
{
    // Implemented on the C++/CLI side by MyPluginUiGlue::MyPluginControllerBridge, which
    // wraps the native IUnoPluginController&. Add more members here as your UI (or the
    // future serial/pipe/UDP worker) needs more of the SDRuno controller API -- mirror
    // every addition with a matching pass-through method on MyPluginControllerBridge.
    //
    // Keeping this interface (rather than a concrete class) in the pure-C# project is
    // what avoids a circular project reference between MyPluginUi and MyPluginUiGlue:
    // MyPluginUiGlue references MyPluginUi (for MainForm) and implements this interface;
    // MyPluginUi never needs to reference MyPluginUiGlue at all.
    public interface IMyPluginController
    {
        double GetVfoFrequency(int channel);
        bool SetVfoFrequency(int channel, double frequencyHz);

        double GetCenterFrequency(int channel);
        bool SetCenterFrequency(int channel, double frequencyHz);

        int GetFilterBandwidth(int channel);
        bool SetFilterBandwidth(int channel, int bandwidthHz);

        bool IsStreamingEnabled(int channel);

        bool SetAudioVolume(int channel, int volume);
        int GetAudioVolume(int channel);
        bool SetAudioMute(int channel, bool mute);
        bool GetAudioMute(int channel);

        double GetSNR(int channel);
        double GetPower(int channel);

        string GetConfigurationKey(string key);
        bool SetConfigurationKey(string key, string value);
    }
}
