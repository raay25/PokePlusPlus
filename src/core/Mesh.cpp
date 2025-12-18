#include <pokeapp/Mesh.h>

/*
    Implementation of the Mesh class for managing 3D mesh data and rendering.
*/

using namespace pokepp;

Mesh::Mesh(const std::vector<Vertex>& v, const std::vector<unsigned>& i)
    : vertices_(v), indices_(i) {
    setup();
}

Mesh::~Mesh() {
    if (EBO_) glDeleteBuffers(1, &EBO_);
    if (VBO_) glDeleteBuffers(1, &VBO_);
    if (VAO_) glDeleteVertexArrays(1, &VAO_);
}

Mesh::Mesh(Mesh&& o) noexcept { // Move constructor
    *this = std::move(o);
}

Mesh& Mesh::operator=(Mesh&& o) noexcept { // Move assignment
    if (this == &o) return *this;
    std::swap(VAO_, o.VAO_);
    std::swap(VBO_, o.VBO_);
    std::swap(EBO_, o.EBO_);
    vertices_ = std::move(o.vertices_);
    indices_ = std::move(o.indices_);
    return *this;
}

// Setup OpenGL buffers and attribute pointers
void Mesh::setup() {
	// Create OpenGL buffers/arrays
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glGenBuffers(1, &EBO_);

	// Send vertex and index data to GPU
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned), indices_.data(), GL_STATIC_DRAW);

    // layout(location=0) position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    
    // layout(location=1) normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
    // layout(location=3) tex coordinates
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex));

    glBindVertexArray(0);
}

// Called every frame to draw the mesh
void Mesh::draw() const {
    glBindVertexArray(VAO_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
