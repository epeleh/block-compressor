SOURCE_DIR=src
TARGET_DIR=build
HEADERS=$(wildcard $(SOURCE_DIR)/*.h)
SOURCES=$(wildcard $(SOURCE_DIR)/*.c)
OBJECTS=$(SOURCES:$(SOURCE_DIR)/%.c=$(TARGET_DIR)/%.o)
EXECUTABLE=$(TARGET_DIR)/bc

$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(TARGET_DIR)
	gcc -Wall $(OBJECTS) -o $@

$(TARGET_DIR)/%.o: $(SOURCE_DIR)/%.c
	mkdir -p $(TARGET_DIR)
	gcc -Wall -c -o $@ $<

clean:
	rm -rf $(TARGET_DIR)

format:
	clang-tidy -fix -fix-errors \
		--checks=readability-braces-around-statements,misc-macro-parent \
		$(HEADERS) $(SOURCES) --

	clang-format -i $(HEADERS) $(SOURCES)
