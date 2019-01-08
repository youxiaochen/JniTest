package you.jnitest;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        JniTest test = JniTest.createBean("aa", "bb");
        String sha1 = JniTest.getSha1();

        Log.i("you", test.a+"  "+test.b+"  "+sha1);

    }

}
