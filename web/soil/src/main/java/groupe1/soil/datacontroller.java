package groupe1.soil;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api")
public class datacontroller {

    @Autowired
    private dataservice dataService;

    @GetMapping("/read-and-write")
    public String readAndWriteData() {
        dataService.readAndWriteDataToFile();
        return "Data read from the database and written to file successfully!";
    }
}
