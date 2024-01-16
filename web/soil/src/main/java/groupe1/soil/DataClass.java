package groupe1.soil;
import javax.persistence.Entity;
import javax.persistence.Id;

@Entity
public class DataClass {
    @Id
    private Long id;
    private String name;
    private int age;

    public DataClass() {
        // Default constructor required by JPA
    }

    public DataClass(Long id, String name, int age) {
        this.id = id;
        this.name = name;
        this.age = age;
    }

    // Getters and Setters

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public int getAge() {
        return age;
    }

    public void setAge(int age) {
        this.age = age;
    }
}

