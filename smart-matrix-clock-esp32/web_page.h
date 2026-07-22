#pragma once

// Self-contained HTML/CSS/JS web page served by GET /.
// Defined as a raw-string literal so embedded quotes / special chars are safe.
// The page communicates exclusively via fetch() against the ESP32's REST API.

extern const char WEB_PAGE_HTML[];
