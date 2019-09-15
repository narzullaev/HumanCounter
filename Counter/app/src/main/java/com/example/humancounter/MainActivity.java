package com.example.humancounter;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.app.AlertDialog;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.tasks.OnCanceledListener;
import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import dmax.dialog.SpotsDialog;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    private Button mScanButton, mResetButton;
    private TextView mResult;

    private FirebaseUser mFirebaseUser;
    private DatabaseReference mDatabaseReference;

    private List<String> mMacAddresses;
    private Map<String, String> map;

    public AlertDialog dialog;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        dialog = new SpotsDialog.Builder().setContext(MainActivity.this).build();


        // init Firebase Database and Auth
        FirebaseAuth mAuth = FirebaseAuth.getInstance();
        mDatabaseReference = FirebaseDatabase.getInstance().getReference();

        mMacAddresses = new ArrayList<>();

        mScanButton = findViewById(R.id.scanBTN);
        mResetButton = findViewById(R.id.resetBTN);
        mResult = findViewById(R.id.resultTV);

        getMacAddresses();
        setResult();
        resetFields();
    }

    private void resetFields() {
        dialog.setMessage("Resetting. Please wait.");
        mResetButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mResult.setText("No Data");
                mDatabaseReference.child("HumanCounter").removeValue().addOnCompleteListener(new OnCompleteListener<Void>() {
                    @Override
                    public void onComplete(@NonNull Task<Void> task) {
                        dialog.dismiss();
                    }
                }).addOnCanceledListener(new OnCanceledListener() {
                    @Override
                    public void onCanceled() {
                        makeMessage("Resetting failed.");
                    }
                });
            }
        });

        dialog.dismiss();
    }

    @Override
    protected void onStart() {
        super.onStart();
        getMacAddresses();
    }

    @Override
    protected void onResume() {
        super.onResume();
        getMacAddresses();
    }

    private void setResult() {
        mScanButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                getMacAddresses();
                dialog.setMessage("Setting Data...");
                if (map != null) {
                    for (Map.Entry<String, String> entry : map.entrySet()) {
                        // Here we will extract the mac address and will set them to list
                        mMacAddresses.add(entry.getValue());
                    }
                    if (!mMacAddresses.isEmpty()) {

                        // The list contains duplicated mac addresses
                        // Here we will remove the duplicated the values
                        Set<String> newMacSet = new HashSet<>(mMacAddresses);
                        // Clear the current list before adding the new list
                        mMacAddresses.clear();
                        mMacAddresses.addAll(newMacSet);

                        // The list size gives us the number of mac addresses which meanss
                        // the number of photos
                        mResult.setText("" + mMacAddresses.size());
                        dialog.dismiss();
                    }
                } else {
                    makeMessage("No data");
                    dialog.dismiss();
                }

                dialog.dismiss();
            }
        });
    }

    private void makeMessage(String message) {
        // Creates toast message
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    private void getMacAddresses() {
        dialog.setMessage("Loading...");
        dialog.show();
        mDatabaseReference.child("HumanCounter").addListenerForSingleValueEvent(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                // This will get the data from firebase and set to Map
                if (dataSnapshot.getValue()!=null){
                    map = (Map<String, String>) dataSnapshot.getValue();
                    Log.i(TAG, map.toString());
                    dialog.dismiss();
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {
                dialog.dismiss();
            }
        });
        dialog.dismiss();
    }
}
