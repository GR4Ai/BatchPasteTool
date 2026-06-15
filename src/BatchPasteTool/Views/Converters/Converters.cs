using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace BatchPasteTool.Views.Converters;

public class BoolToVisibilityConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        bool b = value is true;
        bool invert = parameter is string s && s == "Invert";
        return (b ^ invert) ? Visibility.Visible : Visibility.Collapsed;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is Visibility v && v == Visibility.Visible;
    }
}

public class InverseBoolConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is bool b)
        {
            // If a double parameter is given (e.g. 0.25), return opacity: true→1.0, false→dimmed
            if (parameter is string s && double.TryParse(s, out double dimOpacity))
                return b ? 1.0 : dimOpacity;
            return !b;
        }

        // If a double parameter is given (e.g. 0.25), return opacity
        if (parameter is string s2 && double.TryParse(s2, out double opacity))
            return (value is true) ? opacity : 1.0;

        return !(value is true);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return value is bool b && !b;
    }
}
