# Computer Animation Competition (Softwaredokumentation)

## Gruppenmitglieder

- Patrick Hähn
- Sebastian Kühn
- Volker Sobek

## Künstlerisches Konzept

## Technisches Konzept

## Arbeitsschritte, Komponenten, Teilergebnisse

### Noise Generation (Perlin Noise)

Die Noise Generation ist für die nachfolgende Terrain Generation essenziell. Wir haben uns für *Perlin Noise* [1] entschieden, da diese Methode für unsere Zwecke hinreichend ist und durch Überlagerung von verschiedenen Frequenzen anschauliche Height-Maps generiert werden können. Die Funktionalität wurde in der Klasse **perlin_noise** gekapselt, die ausschließlich innerhalb der nachfolgenden Terrain Generation verwendet wird.

```cpp
class perlin_noise 
{
	// Private member properties
    float *** gradients;
    int gradients_count;
    float max_distance;
    float offset;
    float scaling;
    float gradient_grid_distance;

	// Private member functions
    void create_gradients();
    float dot_grid_gradient(int index_x, int index_y, float x, float y);
    float lerp(float high, float low, float weight);
	float fade(float x);

public:
	// Public constructor & destructor
    perlin_noise(int gradients_count, float grid_distance, float offset, float scaling);
    ~perlin_noise();

	// Public member functions
    float get_noise(float x, float y);
	void clear_gradients();
};
```

### Terrain Generation

Die Generierung der Terrains sowie dessen Visualisierung werden in einer Klasse **terrain** gekapselt, sodass nach deren Instanziierung lediglich die öffentliche Funktion **terrain::render(int)** in der Render-Loop aufgerufen werden muss. Die Klasse bietet desweiteren eine Funktion **terrain::get_normal_at_pos(float,float)**, die für die physikalischen Berechnungen benötigt wird. Die Generierung des Terrains basiert auf der obigen eigenen Implementierung von **perlin_noise** und wurde inspiriert durch *Red Blob Games* [2].

```cpp
class terrain
{
	// Private member properties
	int start_frame = 0;
	int current_frame = 0;
	int frame_loc;
	int max_frame_loc;
	geometry terra;
	float * heights;
	int frames;
	float size;
	int resolution;
	float min_height = 0.0;
	float max_height = 1.0;
	float lowest_height = 0.0;
	float highest_height = 1.0;

	// Static private class properties
	static int stone_loc;
	static int grass_loc;
	static int snow_loc;

	// Private member functions
	float * get_heights(float range, float rigidity);
	void build();
	void clamp_heights();
	void get_frame_locations(int shader_program);
	void set_frames(int start, int max);
	void reset_current_frame();
	void increase_current_frame(int increase = 1);

	// Static private class functions
	static void get_texture_locations(int shader_program);
	static void load_textures(std::string stone, std::string grass, std::string snow);
	static unsigned create_texture_rgba32f(int width, int height, float* data);
	static float* load_texture_data(std::string filename, int* width, int* height);
	static void set_texture_filter_mode(unsigned int texture, GLenum mode);
	static void set_texture_wrap_mode(unsigned int texture, GLenum mode);

public:
	// Public constructor & destructor
	terrain(float size, int resolution, int start_frame, int max_frame, int shader_program, std::string stone, std::string grass, std::string snow);
	~terrain();

	// Public member functions
	glm::vec3 * get_normal_at_pos(float x, float z);
	void render(int model_loc);
};
```
### Physikalische Simulation

### Kamera Effekte

### Video Rendering

## Ergebnis


## Referenzen

[1] Wikipedia, https://en.wikipedia.org/wiki/Perlin_noise, abgerufen: 05.07.2019
[2] Red Blob Games, https://www.redblobgames.com/maps/terrain-from-noise/, abgerufen: 05.07.2019