#include "ssd1306.hpp"

// private methods
void SSD1306::oled_set_position(uint8_t page, uint8_t column)
{
  Wire.beginTransmission(oled_addr);
  Wire.write(0x00);                          // control byte: command
  Wire.write(0xB0 | (page & 0x07));          // 設置頁地址
  Wire.write(0x00 | (column & 0x0F));        // 設置列地址低位
  Wire.write(0x10 | ((column >> 4) & 0x0F)); // 設置列地址高位
  Wire.endTransmission();
}

void SSD1306::oled_write_page_data(uint8_t page, uint8_t column, const uint8_t *data, size_t len)
{
  if (!data || len == 0)
    return;

  oled_set_position(page, column);

  for (size_t i = 0; i < len; i += I2C_BLOCK_MAX)
  {
    size_t chunk = (len - i > I2C_BLOCK_MAX) ? I2C_BLOCK_MAX : len - i;
    Wire.beginTransmission(oled_addr);
    Wire.write(0x40); // control byte: data
    Wire.write(&data[i], chunk);
    Wire.endTransmission();
  }
  return;
}

static void __expand_4_bits_to_8(uint8_t data, uint8_t shift, uint8_t *result)
{
  uint8_t bits = (data >> shift) & 0x0F;
  *result = 0;
  for (uint8_t bit = 0; bit < 4; bit++)
  {
    uint8_t bit_value = (bits >> bit) & 1;
    *result |= (bit_value << (bit * 2));
    *result |= (bit_value << (bit * 2 + 1));
  }
}

static void __expand_font(uint8_t font_index, uint8_t *upper, uint8_t *lower)
{
  // font_5x7 每個字元有 5 個 byte，展開後各 10 個 byte (每行重複一次)
  for (uint8_t i = 0; i < 5; i++)
  {
    uint8_t row = font_5x7[font_index][i];
    uint8_t val;

    __expand_4_bits_to_8(row, 0, &val);
    upper[i * 2] = val;
    upper[i * 2 + 1] = val;

    __expand_4_bits_to_8(row, 4, &val);
    lower[i * 2] = val;
    lower[i * 2 + 1] = val;
  }
}

void SSD1306::print_chr(uint8_t page, uint8_t column, char data, bool is_number)
{
  uint8_t font_index;

  if (is_number)
  {
    font_index = (uint8_t)data + 0x10; // 數字偏移 +16
  }
  else
  {
    font_index = (uint8_t)data - 0x20; // 字庫偏移 -32
  }

  uint8_t upper[10];
  uint8_t lower[10];
  __expand_font(font_index, upper, lower);

  oled_write_page_data(page, column, upper, 10);
  oled_write_page_data(page + 1, column, lower, 10);

  return;
}

// runtime methods
void SSD1306::begin()
{
  if (active)
    return; // 防止重複啟動
  active = true;

  xTaskCreatePinnedToCore(
      [](void *obj)
      { static_cast<SSD1306 *>(obj)->run(); }, // Trampoline
      "SSD1306_Task",                          // 任務名稱
      2048,                                    // Stack 大小
      this,                                    // 傳入物件指標
      1,                                       // 優先級 (0 為最低)
      &taskHandle,                             // 任務控制把手
      1                                        // 指定跑在 Core 1
  );
}

void SSD1306::run()
{

  ssd1306_init();
  int reverse_count = 0;
  // user code here

  //=================
  while (active)
  {
    if (reverse_count++ > reverse_threshold)
    {
      ssd1306_reverse();
      reverse_count = 0;
    }
    else if (reverse_count == 0)
    {
      ssd1306_reverse();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  vTaskDelete(NULL);
}

void SSD1306::stop()
{
  active = false;
}

// public methods
void SSD1306::ssd1306_init()
{
  Wire.begin();
  Wire.beginTransmission(oled_addr);
  Wire.write(0x00); // Control byte for commands
  for (size_t i = 0; i < sizeof(inital_commands); i++)
  {
    Wire.write(inital_commands[i]);
  }
  Wire.endTransmission();
}

void SSD1306::ssd1306_full_light()
{
  for (uint8_t page = 0; page < 8; page++)
  {
    Wire.beginTransmission(oled_addr);
    Wire.write(0x00);        // Control byte for commands
    Wire.write(0xB0 | page); // Set page address
    Wire.write(0x00);        // Set column address low nibble
    Wire.write(0x10);        // Set column address high nibble
    Wire.endTransmission();

    Wire.beginTransmission(oled_addr);
    Wire.write(0x40); // Control byte for data
    for (uint8_t col = 0; col < 128; col++)
    {
      Wire.write(0xFF); // Set pixel data to fully lit for the entire page
    }
    Wire.endTransmission();
  }
}

void SSD1306::ssd1306_reverse()
{
  Wire.beginTransmission(oled_addr);
  Wire.write(0x00);
  Wire.write(0xA7); // Set inverse display
  Wire.endTransmission();
}

void SSD1306::ssd1306_shift(int8_t y_shift = 0)
{
  // this function is for y axis shift, x axis shift can be implemented later with bit shift
  Wire.beginTransmission(oled_addr);
  Wire.write(0x00);
  Wire.write(0xD3);           // Set display offset
  Wire.write(y_shift & 0x3F); // Y 軸偏移量
  Wire.endTransmission();
}

void SSD1306::ssd1306_clear()
{
  for (uint8_t page = 0; page < 8; page++)
  {
    Wire.beginTransmission(oled_addr);
    Wire.write(0x00);        // Control byte for commands
    Wire.write(0xB0 | page); // Set page address
    Wire.write(0x00);        // Set column address low nibble
    Wire.write(0x10);        // Set column address high nibble
    Wire.endTransmission();

    Wire.beginTransmission(oled_addr);
    Wire.write(0x40); // Control byte for data
    for (uint8_t col = 0; col < 128; col++)
    {
      Wire.write(0x00); // Clear pixel data for the entire page
    }
    Wire.endTransmission();
  }
}

void SSD1306::display_string(uint8_t start_page, uint8_t start_column, const char *text)
{
  uint8_t column = start_column;
  uint8_t page = start_page;

  const uint8_t char_width = 10;
  const uint8_t char_spacing = 1;

  for (size_t i = 0; text[i] != '\0'; i++)
  {
    print_chr(page, column, text[i], false);
    column += (char_spacing + char_width);

    if (column > 118)
    {
      column = start_column;
      page += 2;

      if (page >= 7)
        page = start_page;
    }
  }
}

void SSD1306::display_icons(uint8_t start_page, uint8_t start_col, const uint8_t icon[4][8])
{
  oled_write_page_data(start_page, start_col, icon[0], 8);
  oled_write_page_data(start_page, start_col + 8, icon[1], 8);
  oled_write_page_data(start_page + 1, start_col, icon[2], 8);
  oled_write_page_data(start_page + 1, start_col + 8, icon[3], 8);
}

void SSD1306::display_image(uint8_t page, uint8_t col, const uint8_t *data)
{
  uint16_t idx = 0;

  for (uint8_t p = 0; p < 6; p++)
  {
    oled_write_page_data(page + p, col, &data[idx], 64);
    idx += 64;
  }
}
