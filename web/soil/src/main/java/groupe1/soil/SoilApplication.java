package groupe1.soil;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.autoconfigure.domain.EntityScan;

@SpringBootApplication
@EntityScan("groupe1.soil")
public class SoilApplication {

	public static void main(String[] args) {
		SpringApplication.run(SoilApplication.class, args);
	}

}


