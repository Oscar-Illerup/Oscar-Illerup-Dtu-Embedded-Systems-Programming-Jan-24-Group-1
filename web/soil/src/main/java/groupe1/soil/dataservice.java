package groupe1.soil;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.io.FileWriter;
import java.io.IOException;
import java.util.List;

@Service
public class dataservice {

    @Autowired
    private dataRepository dataRepository;

    public void readAndWriteDataToFile() {
        List<DataClass> people = dataRepository.findAll();

        try (FileWriter writer = new FileWriter("output.csv")) {
            for (DataClass data_class : people) {
                writer.write(data_class.getId() + "," + data_class.getName() + "," + data_class.getAge() + "\n");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
