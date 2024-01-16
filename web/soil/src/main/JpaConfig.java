package groupe1.soil;

import org.springframework.context.annotation.Configuration;
import org.springframework.data.jpa.repository.config.EnableJpaRepositories;

@Configuration
@EnableJpaRepositories(basePackages = "groupe1.soil")
public class JpaConfig {
    // additional JPA configuration if needed
}

