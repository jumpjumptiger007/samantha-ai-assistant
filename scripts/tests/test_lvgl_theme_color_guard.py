import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
THEME_SOURCE = ROOT / "main" / "display" / "lvgl_display" / "lvgl_theme.cc"


class LvglThemeColorGuardTest(unittest.TestCase):
    def test_fixed_rgb_substrings_are_guarded_by_full_color_length(self):
        source = THEME_SOURCE.read_text(encoding="utf-8")
        match = re.search(
            r"lv_color_t LvglTheme::ParseColor\(const std::string& color\) \{(?P<body>.*?)\n\}",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(match)
        body = match.group("body")

        guard = "if (color.size() >= 7 && color[0] == '#') {"
        self.assertIn(guard, body)
        self.assertIn("color.substr(1, 2)", body)
        self.assertIn("color.substr(3, 2)", body)
        self.assertIn("color.substr(5, 2)", body)
        self.assertIn("return lv_color_black();", body)


if __name__ == "__main__":
    unittest.main()
