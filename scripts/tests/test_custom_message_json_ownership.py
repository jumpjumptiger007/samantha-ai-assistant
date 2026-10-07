import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
APPLICATION_SOURCE = ROOT / "main" / "application.cc"


class CustomMessageJsonOwnershipTest(unittest.TestCase):
    def test_custom_message_print_buffers_are_owned_and_payload_is_guarded(self):
        source = APPLICATION_SOURCE.read_text(encoding="utf-8")
        self.assertIn('#include "cjson_utils.h"', source)

        start = source.index("#if CONFIG_RECEIVE_CUSTOM_MESSAGE")
        end = source.index("#endif", start)
        branch = source[start:end]

        self.assertIn(
            "CJsonStringUniquePtr root_json(cJSON_PrintUnformatted(root));", branch
        )
        self.assertIn(
            'ESP_LOGI(TAG, "Received custom message: %s", root_json ? root_json.get() : "");',
            branch,
        )
        self.assertIn(
            "CJsonStringUniquePtr payload_json(cJSON_PrintUnformatted(payload));", branch
        )
        payload_guard = branch.index("if (payload_json)")
        schedule = branch.index("Schedule(", payload_guard)
        payload_copy = branch.index("std::string(payload_json.get())", schedule)
        display_update = branch.index(
            'display->SetChatMessage("system", payload_str.c_str())', payload_copy
        )
        self.assertLess(payload_guard, schedule)
        self.assertLess(schedule, payload_copy)
        self.assertLess(payload_copy, display_update)
        self.assertIn('ESP_LOGW(TAG, "Invalid custom message format: missing payload");', branch)

        self.assertNotIn(
            'ESP_LOGI(TAG, "Received custom message: %s", cJSON_PrintUnformatted(root));',
            branch,
        )
        self.assertNotIn("std::string(cJSON_PrintUnformatted(payload))", branch)


if __name__ == "__main__":
    unittest.main()
